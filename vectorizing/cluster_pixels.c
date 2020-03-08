#include "headers.h"


typedef struct pixel_sample_data pixel_sample_data;
typedef struct adjacent_pixel_cluster_tree_node adjacent_pixel_cluster_tree_node;
typedef struct pixel_cluster pixel_cluster;

/// adjacent offset directions
///
///  3 2 1
///   \|/
///  4-X-0
///   /|\
///  5 6 7

//static const uint32_t directly_adjacent_pixel_samples=4;
static const uint32_t adjacent_pixel_samples=8;

//static pixel_sample_data * test_psd=NULL;


struct pixel_cluster
{
    adjacent_pixel_cluster_tree_node * tree_root;

    pixel_sample_data * first_internal_pixel;
    pixel_sample_data * first_border_pixel;

    uint32_t pixel_count;
    uint32_t times_altered;
    uint32_t identifier;
    uint64_t total_contribution_factor;

    uint32_t tested:1;
    //uint32_t in_list:1;

    pixel_lab colour;///average

    double l_total,a_total,b_total;

    pixel_cluster * next;
    pixel_cluster * prev;

//    pixel_cluster * next_changed;///used in temporary stacks e.g. changed clusters that need re-assessment (when collapsing clusters)

    face * f;
};

typedef struct unresolved_diagonal
{
    int32_t x,y;

    int32_t v;

    pixel_cluster *tr_bl_cluster,*tl_br_cluster;
}
unresolved_diagonal;


struct pixel_sample_data
{
    pixel_cluster * cluster;

    /// in cluster
    pixel_sample_data * next;
    pixel_sample_data * prev;

    ///pixel_sample_data * adjacent_pixels[9];///null terminated so needs space for end null and 8 possible adjacents

    uint32_t auto_combine_identifier;///tmp_cluster_index
//    uint32_t rgb_hash:24;///8 bit concatenation or RGB values for this pixel, used for automatic combination
    uint32_t adjacent_sample_valid:8;
    uint32_t contribution_factor:4;///effectively number of adjacent pixel_samples from same cluster
    uint32_t moved:1;
    uint32_t border:1;

    pixel_lab colour;
};


//static pixel_sample_data * test_pixel;

///bottle neck still remains cie comparison, perhaps compare cie76 to cie00 factor (esp for pixels) to allow faster clusterisation

struct adjacent_pixel_cluster_tree_node
{
    adjacent_pixel_cluster_tree_node * left;
    adjacent_pixel_cluster_tree_node * right;
    adjacent_pixel_cluster_tree_node * parent;

    //bool is_red;
    int32_t relative_height;

    pixel_cluster * c;
};

typedef struct cluster_border_pixel
{
    pixel_cluster * pc;
    pixel_sample_data * psd;
}
cluster_border_pixel;

typedef struct cluster_change ///either combination or shift pixel from one to the other (adjust step)
{
    pixel_cluster * pc1;
    pixel_cluster * pc2;

    pixel_sample_data * psd;///used when recycling for moving pixel to another cluster, in that case pc1 is old cluster and pc2 is new cluster

    uint32_t pc1_times_altered;
    uint32_t pc2_times_altered;

    float difference;
}
cluster_change;


static pixel_cluster * get_pixel_cluster(pixel_cluster ** first_available_cluster,pixel_cluster ** first_cluster)
{
    static uint32_t identifier=0;

    pixel_cluster * pc;

    if((pc = *first_available_cluster)) *first_available_cluster=pc->next;
    else
    {
        pc=malloc(sizeof(pixel_cluster));
        pc->times_altered=0;
    }

    pc->prev=NULL;
    if((pc->next = *first_cluster)) (*first_cluster)->prev=pc;
    *first_cluster=pc;

    pc->identifier=identifier++;

    pc->colour.l=pc->colour.a=pc->colour.b=0.0;

    pc->l_total=pc->a_total=pc->b_total=0.0;
    pc->pixel_count=0;
    pc->total_contribution_factor=0;

    pc->tested=false;

    pc->f=NULL;
    pc->first_internal_pixel=NULL;
    pc->first_border_pixel=NULL;
    pc->tree_root=NULL;
//    pc->next_changed=NULL;

    return pc;
}

static void relinquish_pixel_cluster(pixel_cluster ** first_available_cluster,pixel_cluster * pc)
{
    pc->times_altered++;
    pc->next= *first_available_cluster;
    *first_available_cluster=pc;
}

static void add_cluster_change(cluster_change * changes,uint32_t d,cluster_change cc)
{
    uint32_t u;

    while(d)
    {
        u=(d-1)>>1;
        if(cc.difference >= changes[u].difference)break;
        changes[d]=changes[u];
        d=u;
    }

    changes[d]=cc;
}

static cluster_change get_cluster_change(cluster_change * changes,uint32_t c)
{
    uint32_t d,u=0;
    cluster_change cc=changes[0];

    while((d=((u<<1)+1))<c)
    {
        d+=(((d+1)<c)&&(changes[d].difference >= changes[d+1].difference));
        if(changes[d].difference >= changes[c].difference)break;
        changes[u]=changes[d];
        u=d;
    }
    changes[u]=changes[c];

    return cc;
}


#warning use bool to prevent double add attempts and to ensure paired call return same value
static bool add_adjacent_to_cluster(pixel_cluster * parent_cluster,pixel_cluster * adjacent_cluster,adjacent_pixel_cluster_tree_node ** first_available_adjacent)
{
    adjacent_pixel_cluster_tree_node *n,*p,*g;

    if((n= *first_available_adjacent))*first_available_adjacent=n->parent;
    else n=malloc(sizeof(adjacent_pixel_cluster_tree_node));

    p=parent_cluster->tree_root;

    n->left=NULL;
    n->right=NULL;
    n->relative_height=0;
    n->c=adjacent_cluster;

    if(p==NULL)
    {
        n->parent=NULL;
        parent_cluster->tree_root=n;
        return true;
    }

    for(;;)
    {
        if(adjacent_cluster > p->c)
        {
            if(p->right)p=p->right;
            else
            {
                p->right=n;
                p->relative_height++;
                break;
            }
        }
        else if(adjacent_cluster < p->c)
        {
            if(p->left)p=p->left;
            else
            {
                p->left=n;
                p->relative_height--;
                break;
            }
        }
        else
        {
            ///put back in available LL
            n->parent= *first_available_adjacent;
            *first_available_adjacent=n;
            return false;
        }
    }

    n->parent=p;
    g=p->parent;

    while((g)&&(p->relative_height))
    {
        if(p==g->right)
        {
            if(g->relative_height>0)///==1, yet to be updated, ergo "real" relative_height==2
            {
                if(p->relative_height>0)///==1, same as p->right==n b/c p->relative_height cannot be 0 and addition must have been made to n's side (test though)
                {
                    g->relative_height=p->relative_height=0;

                    if((g->right=p->left))p->left->parent=g;

                    p->parent=g->parent;
                    g->parent=p;

                    p->left=g;
                }
                else ///p->relative_height = -1,same as p->left==n
                {
                    p->relative_height=  (n->relative_height<0);
                    g->relative_height= -(n->relative_height>0);
                    n->relative_height= 0;

                    if((p->left=n->right))n->right->parent=p;
                    if((g->right=n->left))n->left->parent=g;

                    n->parent=g->parent;
                    g->parent=p->parent=n;

                    n->right=p;
                    n->left=g;

                    p=n;
                }

                if(p->parent)
                {
                    if(p->parent->left==g)p->parent->left=p;
                    else p->parent->right=p;
                }
                else parent_cluster->tree_root=p;

                break;
            }
            else g->relative_height++;
        }
        else///p==g->left
        {
            if(g->relative_height<0)
            {
                if(p->relative_height<0)///==-1, same as p->left==n b/c p->relative_height cannot be 0 and addition must have been made to n's side (test though)
                {
                    g->relative_height=p->relative_height=0;

                    if((g->left=p->right))p->right->parent=g;

                    p->parent=g->parent;
                    g->parent=p;

                    p->right=g;
                }
                else///p->relative_height = 1,same as p->right==n
                {
                    p->relative_height= -(n->relative_height>0);
                    g->relative_height=  (n->relative_height<0);
                    n->relative_height= 0;

                    if((p->right=n->left))n->left->parent=p;
                    if((g->left=n->right))n->right->parent=g;

                    n->parent=g->parent;
                    g->parent=p->parent=n;

                    n->left=p;
                    n->right=g;

                    p=n;
                }

                if(p->parent)
                {
                    if(p->parent->left==g)p->parent->left=p;
                    else p->parent->right=p;
                }
                else parent_cluster->tree_root=p;

                break;
            }
            else g->relative_height--;
        }

        n=p;
        p=g;
        g=g->parent;
    }

    return true;
}

static void remove_adjacent_cluster_reference(pixel_cluster * parent_cluster,pixel_cluster * adj_cluster,adjacent_pixel_cluster_tree_node ** first_available_adjacent)
{
    adjacent_pixel_cluster_tree_node *n,*p,*s,*r;

    r=parent_cluster->tree_root;

    while((r)&&(r->c!=adj_cluster))///works even if starting r is NULL
    {
        if(r->c > adj_cluster) r=r->left;
        else r=r->right;
    }

    if(r==NULL)
    {
        puts("VALUE NOT FOUND IN AVL TREE");
        return;
    }

    if(r->relative_height>0)
    {
        for(n=r->right;n->left;n=n->left);///removed->right left most

        r->c=n->c;
        if((r=n->right)) n->c=r->c;
        else r=n;
    }
    else if(r->left)
    {
        for(n=r->left;n->right;n=n->right);///removed->left right most

        r->c=n->c;
        if((r=n->left))n->c=r->c;
        else r=n;
    }
    ///else r itself is leaf node (no children)

    n=r;
    p=n->parent;

    if(p==NULL)
    {
        parent_cluster->tree_root=NULL;

        ///put removed value back in available LL
        r->parent= *first_available_adjacent;
        *first_available_adjacent=r;
        return;
    }

    while(p)///balance with assumption that branch with n is 1 shorter than previously
    {
        if(p->left==n)///n is side from which one is removed
        {
            if(p->relative_height>0)
            {
                s=p->right;///must exist
                if(s->relative_height<0)///cannot break, (p's replacement(n)'s height is always 1 lower)
                {
                    n=s->left;///must exist, current value of n is finished being used anyway , is s(sibling)'s child

                    p->relative_height= -(n->relative_height>0);
                    s->relative_height=  (n->relative_height<0);
                    n->relative_height=0;

                    if((p->right=n->left))n->left->parent=p;
                    if((s->left=n->right))n->right->parent=s;

                    n->parent=p->parent;
                    p->parent=s->parent=n;

                    n->left=p;
                    n->right=s;
                }
                else
                {
                    p->relative_height= -(--(s->relative_height));

                    if((p->right=s->left))s->left->parent=p;

                    s->parent=p->parent;
                    p->parent=s;

                    s->left=p;

                    n=s;
                }

                if(n->parent)
                {
                    if(n->parent->left==p)n->parent->left=n;
                    else n->parent->right=n;
                }
            }
            else
            {
                p->relative_height++; /// if p->relative_height is 1 now break condition met (p's height unchanged)
                n=p;
            }
        }
        else /// p->right==n
        {
            if(p->relative_height<0)
            {
                s=p->left;///must exist
                if(s->relative_height>0)
                {
                    n=s->right;///must exist, current value of n is finished being used anyway , is s(sibling)'s child

                    p->relative_height=  (n->relative_height<0);
                    s->relative_height= -(n->relative_height>0);
                    n->relative_height=0;

                    if((p->left=n->right))n->right->parent=p;
                    if((s->right=n->left))n->left->parent=s;

                    n->parent=p->parent;
                    p->parent=s->parent=n;

                    n->right=p;
                    n->left=s;
                }
                else
                {
                    p->relative_height= -(++(s->relative_height));

                    if((p->left=s->right))s->right->parent=p;

                    s->parent=p->parent;
                    p->parent=s;

                    s->right=p;

                    n=s;
                }

                if(n->parent)
                {
                    if(n->parent->left==p)n->parent->left=n;
                    else n->parent->right=n;
                }
            }
            else
            {
                p->relative_height--;
                n=p;
            }
        }

        p=n->parent;

        if(n->relative_height) break;
    }

    if(p==NULL) parent_cluster->tree_root=n;

    p=r->parent;
    if(r==p->left)p->left=NULL;
    else p->right=NULL;

    ///put removed value back in available LL
    r->parent= *first_available_adjacent;
    *first_available_adjacent=r;
}



static void add_pixel_to_cluster(pixel_cluster * pc,pixel_sample_data * psd,int32_t * adjacent_offsets)
{
    pixel_sample_data * ap;
    uint32_t i;
    float f;


    psd->contribution_factor=0;
    psd->cluster=pc;

    pc->pixel_count++;

    psd->next=pc->first_internal_pixel;
    psd->prev=NULL;

    if(pc->first_internal_pixel)pc->first_internal_pixel->prev=psd;
    pc->first_internal_pixel=psd;

    for(i=0;i<adjacent_pixel_samples;i++)
    {
        if(psd->adjacent_sample_valid & 1<<i)
        {
            ap=psd+adjacent_offsets[i];

            if(ap->cluster==pc)
            {
                f=(float)(1<<ap->contribution_factor);
                pc->total_contribution_factor+= 1<<ap->contribution_factor;

                pc->l_total+=ap->colour.l*f;
                pc->a_total+=ap->colour.a*f;
                pc->b_total+=ap->colour.b*f;

                psd->contribution_factor++;
                ap->contribution_factor++;
            }
        }
    }

    f=(float)(1<<psd->contribution_factor);
    pc->total_contribution_factor+= 1<<psd->contribution_factor;

    pc->l_total+=psd->colour.l*f;
    pc->a_total+=psd->colour.a*f;
    pc->b_total+=psd->colour.b*f;
}

static void recalculate_cluster_colour(pixel_cluster * pc)
{
    double f=1.0/((double)pc->total_contribution_factor);

    pc->colour.l=pc->l_total*f;
    pc->colour.a=pc->a_total*f;
    pc->colour.b=pc->b_total*f;

    pc->colour.c=sqrtf(pc->colour.a*pc->colour.a + pc->colour.b*pc->colour.b);
}






static void find_best_pixel_cluster_combination_recursive(cluster_change * cc,adjacent_pixel_cluster_tree_node * adj,float combination_threshold,uint64_t min_valid_cluster_contribution_factor)
{
    double d;

    if(adj->left)find_best_pixel_cluster_combination_recursive(cc,adj->left,combination_threshold,min_valid_cluster_contribution_factor);
    if(adj->right)find_best_pixel_cluster_combination_recursive(cc,adj->right,combination_threshold,min_valid_cluster_contribution_factor);

    //d=cie00_difference_approximation_squared(cc->pc1->colour_lab,adj->c->colour_lab);

    #warning the testing against adjacent clusters best should work, but double check that it doesnt remove cc->pc1 from effective list of clusters to be combined
    ///if one tested against becomes best combination candidate that would be handled on its end
    ///if no adjacent clusters for which this cluster is best candidate then it should be valid to wait for them to judge this cluster as best combination candidate
    ///above actually details decent optimisation strategy

    #warning best combination difference may have changed since this was set (as colour may have changed) ergo the value will no longer be valid

    if(cie00_difference_approximation_squared(cc->pc1->colour,adj->c->colour)<(cc->difference*cc->difference))//&&(d<(adj->c->best_combination_difference*adj->c->best_combination_difference)))
    {
        d=cie00_difference(cc->pc1->colour,adj->c->colour);

        if((d<cc->difference)&&((d<combination_threshold)||(adj->c->total_contribution_factor<min_valid_cluster_contribution_factor)||(cc->pc1->total_contribution_factor<min_valid_cluster_contribution_factor)))//(d < adj->c->best_combination_difference)&&
        {
            cc->difference=d;
            cc->pc2=adj->c;
            cc->pc2_times_altered=adj->c->times_altered;
        }
    }
}

static cluster_change find_best_pixel_cluster_combination(pixel_cluster * pc,float combination_threshold,uint64_t min_valid_cluster_contribution_factor)
{
    cluster_change cc;

    cc.pc1=pc;
    cc.pc1_times_altered=pc->times_altered;

    cc.pc2=NULL;

    cc.difference=1000000.0;

    if(pc->tree_root)find_best_pixel_cluster_combination_recursive(&cc,pc->tree_root,combination_threshold,min_valid_cluster_contribution_factor);

    return cc;
}


static void combine_clusters(pixel_cluster * pc1,pixel_cluster * pc2,adjacent_pixel_cluster_tree_node ** first_available_adjacent,int32_t * adjacent_offsets)
{
    pc1->times_altered++;///pc2 has its value changed upon relinquishment

    adjacent_pixel_cluster_tree_node *adj,*adj_p;
    pixel_cluster * adjacent_pc;
    pixel_sample_data * psd;

    remove_adjacent_cluster_reference(pc1,pc2,first_available_adjacent);
    remove_adjacent_cluster_reference(pc2,pc1,first_available_adjacent);

    bool b1,b2;

    adj=pc2->tree_root;
    while(adj)
    {
        if(adj->left)adj=adj->left;
        else if(adj->right)adj=adj->right;
        else
        {
            adj_p=adj->parent;
            adjacent_pc=adj->c;

            remove_adjacent_cluster_reference(adjacent_pc,pc2,first_available_adjacent);

            adj->parent= *first_available_adjacent;
            *first_available_adjacent=adj;

            b1=add_adjacent_to_cluster(pc1,adjacent_pc,first_available_adjacent);
            b2=add_adjacent_to_cluster(adjacent_pc,pc1,first_available_adjacent);

            if(b1!=b2)puts("PAIRED CLUSTER REFERENCE ADDS HAVE DIFFERENT RETURN VALUE");

            if(adj_p)
            {
                if(adj_p->left==adj)adj_p->left=NULL;
                else adj_p->right=NULL;
            }

            adj=adj_p;
        }
    }
    pc2->tree_root=NULL;

    while((psd=pc2->first_internal_pixel))
    {
        pc2->first_internal_pixel=psd->next;
        add_pixel_to_cluster(pc1,psd,adjacent_offsets);
    }

    recalculate_cluster_colour(pc1);
}

static uint32_t collapse_clusters(pixel_cluster ** first_cluster,uint32_t cluster_count,adjacent_pixel_cluster_tree_node ** first_available_adjacent,pixel_cluster ** first_available_cluster,
                                  cluster_change ** changes,uint32_t * change_space,float combination_threshold,uint64_t min_valid_cluster_contribution_factor,int32_t * adjacent_offsets)
{
    cluster_change cc;
    uint32_t tt,change_count;
    pixel_cluster *pc;//,*first_changed_cluster,*fcc;


    change_count=0;
    tt=SDL_GetTicks();



    for(pc= *first_cluster;pc;pc=pc->next)
    {
        cc=find_best_pixel_cluster_combination(pc,combination_threshold,min_valid_cluster_contribution_factor);

        if(cc.pc2)
        {
            if(change_count== *change_space)*changes=realloc(*changes,sizeof(cluster_change)*(*change_space*=2));
            add_cluster_change(*changes,change_count++,cc);
        }
    }

    while(change_count--)
    {
        cc=get_cluster_change(*changes,change_count);

        if((SDL_GetTicks()-tt)>1000)
        {
            tt=SDL_GetTicks();
            printf("collapse_clusters: %u\n",cluster_count);
        }

        if(cc.pc1_times_altered == cc.pc1->times_altered)
        {
            if(cc.pc2_times_altered == cc.pc2->times_altered)
            {
                if(cc.pc1->pixel_count < cc.pc2->pixel_count)///make sure pc2 provided to combine_clusters has fewer pixels (& thus requires less operations)
                {
                    pc=cc.pc1;
                    cc.pc1=cc.pc2;
                    cc.pc2=pc;
                }

                cluster_count--;

                ///remove pc2 from LL
                if(cc.pc2->next)cc.pc2->next->prev=cc.pc2->prev;
                if(cc.pc2->prev)cc.pc2->prev->next=cc.pc2->next;
                else *first_cluster=cc.pc2->next;


                combine_clusters(cc.pc1,cc.pc2,first_available_adjacent,adjacent_offsets);
                relinquish_pixel_cluster(first_available_cluster,cc.pc2);

                cc=find_best_pixel_cluster_combination(cc.pc1,combination_threshold,min_valid_cluster_contribution_factor);

                if(cc.pc2)
                {
                    if(change_count== *change_space)*changes=realloc(*changes,sizeof(cluster_change)*(*change_space*=2));
                    add_cluster_change(*changes,change_count++,cc);
                }
            }
            else ///pc1 was unchanged but pc2 has changed
            {
                cc=find_best_pixel_cluster_combination(cc.pc1,combination_threshold,min_valid_cluster_contribution_factor);
                if(cc.pc2)add_cluster_change(*changes,change_count++,cc);///no need to ensure space as one entry was removed at start of loop
            }
        }
    }

    return cluster_count;
}


static void find_and_add_pixel_to_move(pixel_cluster * pc,int32_t * adjacent_offsets,cluster_change ** changes,uint32_t * change_space,uint32_t * change_count)
{
    cluster_change cc;
    pixel_sample_data *psd,*ap;
    uint32_t i;
    float difference;

    cc.pc1=pc;
    cc.pc1_times_altered=pc->times_altered;
    cc.pc2=NULL;
    cc.psd=NULL;
    cc.difference=1.0;///min threshold for change

    for(psd=pc->first_border_pixel;psd;psd=psd->next)
    {
        if(! psd->moved)///if pixel has already been moved do not move back
        {
            for(i=0;i<adjacent_pixel_samples;i++) if(psd->adjacent_sample_valid & 1<<i)
            {
                ap=psd+adjacent_offsets[i];

                if(ap->cluster!=pc)
                {
                    if(! ap->border)puts("AP NOT BORDER");
                    if(! psd->border)puts("PSD NOT BORDER");

                    if(! ap->cluster->tested)///dont test pixel against same cluster twice
                    {
                        ap->cluster->tested=true;
                        difference=cie00_difference(psd->colour,ap->cluster->colour)/cie00_difference(psd->colour,pc->colour);///if less than 1 closer to ap->cluster

                        if(difference<cc.difference)
                        {
                            cc.difference=difference;
                            cc.psd=psd;
                            cc.pc2=ap->cluster;
                            cc.pc2_times_altered=ap->cluster->times_altered;
                        }
                    }
                }
            }

            for(i=0;i<adjacent_pixel_samples;i++)if(psd->adjacent_sample_valid & 1<<i) psd[adjacent_offsets[i]].cluster->tested=false;
        }
    }

    if(cc.pc2)
    {
        if(*change_count == *change_space)*changes=realloc(*changes,sizeof(cluster_change)*(*change_space*=2));
        add_cluster_change(*changes,(*change_count)++,cc);
    }

//    if(cc.psd==test_psd)
//    {
//        printf("-> %f\n",cc.difference);
//        printf("%f %f %f\n",cc.psd->colour.l,cc.psd->colour.a,cc.psd->colour.b);
//        printf("%f %f %f\n",cc.pc1->colour.l,cc.pc1->colour.a,cc.pc1->colour.b);
//        printf("%f %f %f\n",cc.pc2->colour.l,cc.pc2->colour.a,cc.pc2->colour.b);
//    }
}

static void adjust_clusters(pixel_cluster ** first_cluster,cluster_change ** changes,uint32_t * change_space,int32_t * adjacent_offsets)
{
    uint32_t i,j,change_count,tt,moved_count,cycle_count;
    float f;
    pixel_cluster *pc,*adj_pc;
    pixel_sample_data *psd,*ap,*psd_next,*psd_prev;
    cluster_change cc;

    change_count=0;
    tt=SDL_GetTicks();

    for(pc= *first_cluster;pc;pc=pc->next)///move border pixels to that LL as appropriate AND find best border combination candidate, pc1 is always provoking cluster
    {
        for(psd=pc->first_internal_pixel;psd;psd=psd_next)
        {
            psd_next=psd->next;
            psd->border=false;

            for(i=0;i<adjacent_pixel_samples;i++) if(psd->adjacent_sample_valid & 1<<i)
            {
                ap=psd+adjacent_offsets[i];

                if(ap->cluster!=pc)///move to (now border) LL if not done already
                {
                    psd->border=true;

                    if(psd->next)psd->next->prev=psd->prev;
                    if(psd->prev)psd->prev->next=psd->next;
                    else pc->first_internal_pixel=psd->next;

                    if((psd->next=pc->first_border_pixel))pc->first_border_pixel->prev=psd;
                    pc->first_border_pixel=psd;

                    break;
                }
            }
        }

        if(pc->first_border_pixel)pc->first_border_pixel->prev=NULL;
        else puts("ERROR NO BORDER FOR CLUSTER");
    }



    moved_count=0;

    for(cycle_count=0;cycle_count<16;cycle_count++)
    {
        for(pc= *first_cluster;pc;pc=pc->next)
        {
            for(psd=pc->first_internal_pixel;psd;psd=psd->next)psd->moved=false;
            for(psd=pc->first_border_pixel;psd;psd=psd->next)psd->moved=false;
        }

        for(pc= *first_cluster;pc;pc=pc->next) find_and_add_pixel_to_move(pc,adjacent_offsets,changes,change_space,&change_count);

        if(change_count==0)break;

        while(change_count)
        {
            cc=get_cluster_change(*changes,--change_count);

            if((SDL_GetTicks()-tt)>1000)
            {
                tt=SDL_GetTicks();
                printf("adjust_clusters: %u %u\n",cycle_count,moved_count);
            }

            if(cc.pc1->times_altered==cc.pc1_times_altered)
            {
                if(cc.pc2->times_altered==cc.pc2_times_altered)
                {
                    moved_count++;

                    ///for now dont alter cluster colour (no recalculate_cluster_colour) to see if moved is really necessary, it shouldn't be!
                    if(cc.psd->cluster != cc.pc1)puts("ERROR MOVED NOT IN PROVOKING CLUSTER");
                    if(! cc.psd->border)puts("ERROR MOVED NOT IN BORDER");

                    ///move psd from pc1 to ps2, change colour totals, contribution totals and retest all adjacent (sharing border line) pixels from both clusters to see that they're now borders (pc1) or no longer borders (pc2)

                    if(cc.psd->next)cc.psd->next->prev=cc.psd->prev;
                    if(cc.psd->prev)cc.psd->prev->next=cc.psd->next;
                    else cc.pc1->first_border_pixel=cc.psd->next;

                    for(i=0;i<adjacent_pixel_samples;i++) if(cc.psd->adjacent_sample_valid & 1<<i)
                    {
                        ap=cc.psd+adjacent_offsets[i];

                        if(ap->cluster==cc.pc1)
                        {
                            ///reduce effect of ap on pc1 due to psd no longer bordering it
                            ap->contribution_factor--;

                            f=(float)(1<<ap->contribution_factor);
                            cc.pc1->total_contribution_factor-= 1<<ap->contribution_factor;

                            cc.pc1->l_total-=ap->colour.l*f;
                            cc.pc1->a_total-=ap->colour.a*f;
                            cc.pc1->b_total-=ap->colour.b*f;

                            if( ! ap->border)///move ap to border
                            {
                                ap->border=true;
                                ///remove from internal LL
                                if(ap->next)ap->next->prev=ap->prev;
                                if(ap->prev)ap->prev->next=ap->next;
                                else cc.pc1->first_internal_pixel=ap->next;
                                ///put in border LL
                                if((ap->next=cc.pc1->first_border_pixel))cc.pc1->first_border_pixel->prev=ap;
                                ap->prev=NULL;
                                cc.pc1->first_border_pixel=ap;
                            }
                        }
                    }


                    ///remove psd's effect on pc1
                    f=(float)(1<<cc.psd->contribution_factor);
                    cc.pc1->total_contribution_factor-= 1<<cc.psd->contribution_factor;

                    cc.pc1->l_total-=cc.psd->colour.l*f;
                    cc.pc1->a_total-=cc.psd->colour.a*f;
                    cc.pc1->b_total-=cc.psd->colour.b*f;


                    cc.psd->contribution_factor=0;
                    cc.psd->moved=true;
                    cc.psd->cluster=cc.pc2;


                    ///put in pc2 including contribution factor &c.
                    if((cc.psd->next=cc.pc2->first_border_pixel))cc.pc2->first_border_pixel->prev=cc.psd;
                    cc.psd->prev=NULL;
                    cc.pc2->first_border_pixel=cc.psd;

                    for(i=0;i<adjacent_pixel_samples;i++) if(cc.psd->adjacent_sample_valid & 1<<i)
                    {
                        ap=cc.psd+adjacent_offsets[i];

                        if(ap->cluster==cc.pc2)
                        {
                            f=(float)(1<<ap->contribution_factor);
                            cc.pc2->total_contribution_factor+= 1<<ap->contribution_factor;

                            cc.pc2->l_total+=ap->colour.l*f;
                            cc.pc2->a_total+=ap->colour.a*f;
                            cc.pc2->b_total+=ap->colour.b*f;

                            cc.psd->contribution_factor++;
                            ap->contribution_factor++;

                            if(ap->border)
                            {
                                for(j=0;j<adjacent_pixel_samples;j++) if((ap->adjacent_sample_valid & 1<<j)&&(ap[adjacent_offsets[j]].cluster!=cc.pc2))break;

                                if(j==adjacent_pixel_samples)///no longer border, as none of border sharing adjacent pixels are not of the same cluster   ergo remove ap from border LL and add to internal LL
                                {
                                    ap->border=false;

                                    if(ap->next)ap->next->prev=ap->prev;
                                    if(ap->prev)ap->prev->next=ap->next;
                                    else cc.pc2->first_border_pixel=ap->next;

                                    if((ap->next=cc.pc2->first_internal_pixel))cc.pc2->first_internal_pixel->prev=ap;
                                    ap->prev=NULL;
                                    cc.pc2->first_internal_pixel=ap;
                                }
                            }
                        }
                    }

                    f=(float)(1<<cc.psd->contribution_factor);
                    cc.pc2->total_contribution_factor+= 1<<cc.psd->contribution_factor;

                    cc.pc2->l_total+=cc.psd->colour.l*f;
                    cc.pc2->a_total+=cc.psd->colour.a*f;
                    cc.pc2->b_total+=cc.psd->colour.b*f;

                    recalculate_cluster_colour(cc.pc1);
                    recalculate_cluster_colour(cc.pc2);

                    cc.pc1->times_altered++;
                    cc.pc2->times_altered++;

                    find_and_add_pixel_to_move(cc.pc1,adjacent_offsets,changes,change_space,&change_count);
                    find_and_add_pixel_to_move(cc.pc2,adjacent_offsets,changes,change_space,&change_count);
                }
                else find_and_add_pixel_to_move(cc.pc1,adjacent_offsets,changes,change_space,&change_count);///find best entry for cc.pc1 again as old one to move to no longer valid
            }
        }
    }

    printf("adjustment cycles: %u\n",cycle_count);

    for(pc= *first_cluster;pc;pc=pc->next)///move border pixels to that LL as appropriate AND find best border combination candidate, pc1 is always provoking cluster
    {
        psd_prev=NULL;
        for(psd=pc->first_border_pixel;psd;psd=psd->next)
        {
            psd->border=false;
            psd_prev=psd;
        }

        if(pc->first_internal_pixel)pc->first_internal_pixel->prev=psd_prev;
        if(psd_prev)
        {
            psd_prev->next=pc->first_internal_pixel;
            pc->first_internal_pixel=pc->first_border_pixel;
            pc->first_border_pixel=NULL;
        }
    }
}



static int32_t calculate_unresolved_diagonal_value(diagonal_sample_connection_type * diagonals,pixel_sample_data * pixels,pixel_cluster * pc,diagonal_sample_connection_type diagonal_direction,
                                                   int32_t dx,int32_t dy,int32_t x,int32_t y,int32_t w,int32_t h,uint32_t d1,uint32_t d2,uint32_t d_prev,int32_t sample_index)
{
    ///d1=preferred direction,d2=secondary direction
    int32_t v;
    uint32_t d,s1,s2,s3,sd;

    int32_t direction_x[4]={1,0,-1,0};
    int32_t direction_y[4]={0,-1,0,1};

    int32_t x_sample_loc[4]={0,0,-1,-1};///offsets of pixels from diagonal location
    int32_t y_sample_loc[4]={0,-1,-1,0};


    ///dx = direction_x[d1]+direction_x[d2]
    ///dx = direction_y[d1]+direction_y[d2]


    if(((x==0)&&(y==0))||((x==0)&&(y==h))||((x==w)&&(y==0))||((x==w)&&(y==h)))return 0;



    /// diagonals either matching diagonal_direction or unresolved, (or possibly only when matching diagonal_direction and terminating on unresolved diagonal?, maybe 1/2 contribution ?)
    if((x==0)||(y==0)||(x==w)||(y==h))
    {
        if(x==w)
        {
            if((diagonal_direction==TR_BL_DIAGONAL)&&(pixels[y*w-1].cluster==pc))d=1;
            else if((diagonal_direction==TL_BR_DIAGONAL)&&(pixels[y*w+w-1].cluster==pc))d=3;
            else return 0;
        }
        if(y==0)
        {
            if((diagonal_direction==TR_BL_DIAGONAL)&&(pixels[x].cluster==pc))d=0;
            else if((diagonal_direction==TL_BR_DIAGONAL)&&(pixels[x-1].cluster==pc))d=2;
            else return 0;
        }
        if(x==0)
        {
            if((diagonal_direction==TR_BL_DIAGONAL)&&(pixels[y*w].cluster==pc))d=3;
            else if((diagonal_direction==TL_BR_DIAGONAL)&&(pixels[y*w-w].cluster==pc))d=1;
            else return 0;
        }
        if(y==h)
        {
            if((diagonal_direction==TR_BL_DIAGONAL)&&(pixels[h*w-w+x-1].cluster==pc))d=2;
            else if((diagonal_direction==TL_BR_DIAGONAL)&&(pixels[h*w-w+x].cluster==pc))d=0;
            else return 0;
        }
    }
    else if(diagonals[y*w+x]==diagonal_direction) d=(d_prev==d1)?d2:d1;
    else if((diagonals[y*w+x]==NO_DIAGONAL)||(diagonals[y*w+x]==BOTH_DIAGONAL))///dont do this for unresolved diagonals? it is effectively making an assumption about which direction is correct
    {
        #warning verify below

        sd=(4+d2-d1)&3;
        if(sd==3)s2=d1;
        else s2=d2;
        s1=(s2+4-sd)&3;
        s3=(s2+sd)&3;

        s1=(y+y_sample_loc[s1])*w+x+x_sample_loc[s1];
        s2=(y+y_sample_loc[s2])*w+x+x_sample_loc[s2];
        s3=(y+y_sample_loc[s3])*w+x+x_sample_loc[s3];

        if((pixels[s1].cluster == pc)&&(pixels[s2].cluster != pc))d=d1;
        else if((pixels[s2].cluster == pc)&&(pixels[s3].cluster != pc))d=d2;
        else return 0;
    }
    else return 0;///no appropriate direction

    ///do update maths
    x+=direction_x[d];
    y+=direction_y[d];

    dx+=direction_x[d];
    dy+=direction_y[d];

    sample_index++;


    v=(sample_index*sample_index*5)/(dx*dx+dy*dy);///5 is man value to ensure all variations up to 4 samples are handled precisely (*13*17 for 5 samples)
    if(sample_index<4)v+=calculate_unresolved_diagonal_value(diagonals,pixels,pc,diagonal_direction,dx,dy,x,y,w,h,d1,d2,d,sample_index);

    return v;
}

static void calculate_diagonals(pixel_sample_data * pixels,diagonal_sample_connection_type * diagonals,unresolved_diagonal * unresolved_diagonals,uint32_t w,uint32_t h)
{
    uint32_t i,px,py,unresolved_diagonal_count;
    int32_t tr_bl_v,tl_br_v,best_delta_v;
    unresolved_diagonal * ud;

    unresolved_diagonal_count=0;

    pixel_sample_data *psd_tr,*psd_tl,*psd_bl,*psd_br;

    for(py=1;py<h;py++)
    {
        for(px=1;px<w;px++)
        {
            psd_tl=pixels+py*w+px-w-1;
            psd_tr=pixels+py*w+px-w;
            psd_bl=pixels+py*w+px-1;
            psd_br=pixels+py*w+px;

            if((psd_tl->cluster==psd_br->cluster)&&(psd_tr->cluster==psd_bl->cluster)&&(psd_tl->cluster!=psd_tr->cluster))
            {
                diagonals[py*w+px]=NO_DIAGONAL;
                unresolved_diagonals[unresolved_diagonal_count++]=(unresolved_diagonal){.x=px,.y=py,.tr_bl_cluster=psd_tr->cluster,.tl_br_cluster=psd_tl->cluster};
            }
            else if((psd_tl->cluster==psd_br->cluster)&&(psd_tl->cluster!=psd_bl->cluster)&&(psd_tl->cluster!=psd_tr->cluster))diagonals[py*w+px]=TL_BR_DIAGONAL;
            else if((psd_tr->cluster==psd_bl->cluster)&&(psd_tr->cluster!=psd_br->cluster)&&(psd_tr->cluster!=psd_tl->cluster))diagonals[py*w+px]=TR_BL_DIAGONAL;
            else if((psd_tl->cluster==psd_br->cluster)||(psd_tr->cluster==psd_bl->cluster))diagonals[py*w+px]=BOTH_DIAGONAL;///test simple solution to allow all 8 samples for contribution factor
            else diagonals[py*w+px]=NO_DIAGONAL;
        }
    }

    /// why does this no longer take into consideration tmp clusters & paths around? is purely geometry better? (probably yes, consider split clusters with "background" path between)


    printf("unresolved diagonals: %u\n",unresolved_diagonal_count);


    while(unresolved_diagonal_count)
    {
        best_delta_v=0;

        for(i=0;i<unresolved_diagonal_count;i++)
        {
            ud=unresolved_diagonals+i;

            ///1st samples are always the same and provided in function calls below

            tr_bl_v= calculate_unresolved_diagonal_value(diagonals,pixels,ud->tr_bl_cluster,TR_BL_DIAGONAL, 1, 0,ud->x+1,ud->y,w,h,1,0,0,1);
            tr_bl_v+=calculate_unresolved_diagonal_value(diagonals,pixels,ud->tr_bl_cluster,TR_BL_DIAGONAL, 0,-1,ud->x,ud->y-1,w,h,0,1,1,1);
            tr_bl_v+=calculate_unresolved_diagonal_value(diagonals,pixels,ud->tr_bl_cluster,TR_BL_DIAGONAL,-1, 0,ud->x-1,ud->y,w,h,3,2,2,1);
            tr_bl_v+=calculate_unresolved_diagonal_value(diagonals,pixels,ud->tr_bl_cluster,TR_BL_DIAGONAL, 0, 1,ud->x,ud->y+1,w,h,2,3,3,1);

            tl_br_v= calculate_unresolved_diagonal_value(diagonals,pixels,ud->tl_br_cluster,TL_BR_DIAGONAL, 1, 0,ud->x+1,ud->y,w,h,3,0,0,1);
            tl_br_v+=calculate_unresolved_diagonal_value(diagonals,pixels,ud->tl_br_cluster,TL_BR_DIAGONAL, 0,-1,ud->x,ud->y-1,w,h,2,1,1,1);
            tl_br_v+=calculate_unresolved_diagonal_value(diagonals,pixels,ud->tl_br_cluster,TL_BR_DIAGONAL,-1, 0,ud->x-1,ud->y,w,h,1,2,2,1);
            tl_br_v+=calculate_unresolved_diagonal_value(diagonals,pixels,ud->tl_br_cluster,TL_BR_DIAGONAL, 0, 1,ud->x,ud->y+1,w,h,0,3,3,1);

            ud->v=tr_bl_v-tl_br_v;

            if(abs(ud->v)>best_delta_v) best_delta_v=abs(ud->v);
        }

        if(best_delta_v)
        {
            i=0;
            while(i<unresolved_diagonal_count)
            {
                ud=unresolved_diagonals+i;

                if(abs(ud->v)==best_delta_v)
                {
                    diagonals[ud->y*w+ud->x]=(ud->v > 0)?TR_BL_DIAGONAL:TL_BR_DIAGONAL;

                    *ud=unresolved_diagonals[--unresolved_diagonal_count];
                }
                else i++;
            }
        }
        else break;///no resolvable diagonals left, all remaining unresolved diagonals are left as NO_DIAGONAL
    }

    //printf("unresolvable diagonal count: %u\n",unresolved_diagonal_count);
}


static void relinquish_adjacent_tree(adjacent_pixel_cluster_tree_node * adj,adjacent_pixel_cluster_tree_node ** first_available_adjacent)
{
    adjacent_pixel_cluster_tree_node * adj_p;

    while(adj)
    {
        if(adj->left)adj=adj->left;
        else if(adj->right)adj=adj->right;
        else
        {
            adj_p=adj->parent;

            adj->parent= *first_available_adjacent;
            *first_available_adjacent=adj;

            if(adj_p)
            {
                if(adj_p->left==adj)adj_p->left=NULL;
                else adj_p->right=NULL;
            }

            adj=adj_p;
        }
    }
}

static void relinquish_all_clusters_and_adjacents(pixel_cluster * first_cluster,pixel_cluster ** first_available_cluster,adjacent_pixel_cluster_tree_node ** first_available_adjacent)
{
    pixel_cluster * next_cluster;

    while(first_cluster)
    {
        next_cluster=first_cluster->next;
        relinquish_adjacent_tree(first_cluster->tree_root,first_available_adjacent);
        relinquish_pixel_cluster(first_available_cluster,first_cluster);
        first_cluster=next_cluster;
    }
}

void cluster_pixels(vectorizer_data * vd)
{
    puts("cluster_pixels");
    uint32_t t;

    uint32_t px,py,i,j,cluster_count,new_cluster_count,pixel_count,border_stack_size,w,h,auto_combine_identifier,change_space;
    uint64_t min_valid_cluster_contribution_factor;

    pixel_sample_data *psd,*ap,*psd_same_cluster_stack;
    pixel_cluster *pc,*first_cluster,*first_available_cluster;
    adjacent_pixel_cluster_tree_node *first_available_adjacent,*adj;
    pixel_cluster dummy_cluster;

    first_cluster=NULL;

    first_available_cluster=NULL;
    first_available_adjacent=NULL;

    bool correct_clusters=vd->correct_clusters;

    #warning use psd_identifier to store rgb_hash



    if((vd->valid_cluster_size_factor)&1)min_valid_cluster_contribution_factor=11<<((vd->valid_cluster_size_factor>>1) -3);///approximates intermediart sqrt(2) for size factor
    else min_valid_cluster_contribution_factor=1<<(vd->valid_cluster_size_factor>>1);

    printf(">> %lu\n",min_valid_cluster_contribution_factor);

    w=vd->w;
    h=vd->h;
    pixel_count=w*h;

    int32_t adjacent_offsets[8]={1,-w+1,-w,-w-1,-1,w-1,w,w+1};///counter-clockwise starting at x+1
    ///  3 2 1
    ///   \|/
    ///  4-X-0
    ///   /|\
    ///  5 6 7

    change_space=1;

    pixel_sample_data * pixels=malloc(sizeof(pixel_sample_data)*pixel_count);
    cluster_change * changes=malloc(sizeof(cluster_change)*change_space);
    cluster_border_pixel * border_stack=malloc(sizeof(cluster_border_pixel)*pixel_count*8);
    unresolved_diagonal * unresolved_diagonals=malloc(sizeof(unresolved_diagonal)*pixel_count);


    float combination_threshold=0.5*((float)(vd->cluster_combination_threshold));

    cluster_count=0;
    border_stack_size=0;
//    test_psd=pixels+367*1920+558;

    for(py=0;py<h;py++)
    {
        for(px=0;px<w;px++)
        {
            psd=pixels+py*w+px;

            psd->cluster=NULL;

            psd->colour=vd->blurred[py*w+px];
            psd->auto_combine_identifier=rgb_hash_from_lab(psd->colour);

            psd->adjacent_sample_valid=0;
            ///share sides
            psd->adjacent_sample_valid|=(px<(w-1))  <<0;
            psd->adjacent_sample_valid|=(py>0)      <<2;
            psd->adjacent_sample_valid|=(px>0)      <<4;
            psd->adjacent_sample_valid|=(py<(h-1))  <<6;
            ///diagonal
            psd->adjacent_sample_valid|=((px<(w-1))&&(py>0))    <<1;
            psd->adjacent_sample_valid|=((px>0)&&(py>0))        <<3;
            psd->adjacent_sample_valid|=((px>0)&&(py<(h-1)))    <<5;
            psd->adjacent_sample_valid|=((px<(w-1))&&(py<(h-1)))<<7;
        }
    }

    //test_pixel=pixels+522*w+707;///================= REMOVE test_pixel ======================

    ///shift+=2;///to make value calculation more accurate, shift cluster colour values by same after "same value" combinations
    t=SDL_GetTicks();


    #warning following and re-clusterisation after calculating diagonals are almost identical, can duplicate code but use simple variant "put these 2 pixels in the same cluster" test
    /// possibly just by copying rgb_hash to identifier (which would make code exactly identical)

    for(i=0;i<pixel_count;i++)
    {
        psd=pixels+i;

        if(psd->cluster==NULL)
        {
            cluster_count++;
            pc = get_pixel_cluster(&first_available_cluster,&first_cluster);

            psd_same_cluster_stack=psd;

            psd->cluster= &dummy_cluster;/// dummy_cluster acts as identifier to stop double adds
            psd->next=NULL;

            auto_combine_identifier=psd->auto_combine_identifier;

            while(psd_same_cluster_stack)
            {
                psd=psd_same_cluster_stack;
                psd_same_cluster_stack=psd_same_cluster_stack->next;

                add_pixel_to_cluster(pc,psd,adjacent_offsets);

                for(j=0;j<adjacent_pixel_samples;j++)
                {
                    if(psd->adjacent_sample_valid & 1<<j)
                    {
                        ap=psd+adjacent_offsets[j];

                        if((ap->cluster==NULL)&&(auto_combine_identifier == ap->auto_combine_identifier))
                        {
                            ap->cluster= &dummy_cluster;/// dummy_cluster acts as identifier to stop double adds
                            ap->next=psd_same_cluster_stack;
                            psd_same_cluster_stack= ap;
                        }
                        else if((ap->cluster!=pc)&&(ap->cluster!= &dummy_cluster)) border_stack[border_stack_size++]=(cluster_border_pixel){.pc=pc,.psd=ap};//,.adjacency_type=test_stack[test_stack_size].type
                    }
                }
            }

            recalculate_cluster_colour(pc);
        }
    }

    while(border_stack_size)
    {
        border_stack_size--;
        add_adjacent_to_cluster(border_stack[border_stack_size].pc,border_stack[border_stack_size].psd->cluster,&first_available_adjacent);
    }

    printf("cluster setup time: %u\n",SDL_GetTicks()-t);


    printf("ICC: %u\n",cluster_count);

    new_cluster_count=collapse_clusters(&first_cluster,cluster_count,&first_available_adjacent,&first_available_cluster,&changes,&change_space,combination_threshold,min_valid_cluster_contribution_factor,adjacent_offsets);
    if(correct_clusters)adjust_clusters(&first_cluster,&changes,&change_space,adjacent_offsets);

    printf("cluster collapse time: %u\n\n",SDL_GetTicks()-t);
    t=SDL_GetTicks();
    printf("cluster count: %u %u\n",cluster_count,new_cluster_count);



    calculate_diagonals(pixels,vd->diagonals,unresolved_diagonals,w,h);

    do
    {
        /// consider putting adjustment here ?
        /// could result in infinite loop?
        /// diagonal blocking below should solve any problems


        cluster_count=0;
        border_stack_size=0;

        for(py=0;py<h;py++)
        {
            for(px=0;px<w;px++)
            {
                psd=pixels+py*w+px;

                psd->auto_combine_identifier=psd->cluster->identifier;

                psd->cluster=NULL;

                psd->adjacent_sample_valid=0;
                ///share sides
                psd->adjacent_sample_valid|=(px<(w-1))  <<0;
                psd->adjacent_sample_valid|=(py>0)      <<2;
                psd->adjacent_sample_valid|=(px>0)      <<4;
                psd->adjacent_sample_valid|=(py<(h-1))  <<6;
                ///diagonal
                ///ONLY IF DIAGONAL IS DISTINCTLY INCORRECT; PREVENT DAIGONAL FROM BEING CONSIDERED
                ///needs to split when clusters aren't same type, have special case, no diagonals, both diagonals??
//                psd->adjacent_sample_valid|=((px<(w-1))&&(py>0)&&(vd->diagonals[py*w+px+1]==TR_BL_DIAGONAL))        <<1;
//                psd->adjacent_sample_valid|=((px>0)&&(py>0)&&(vd->diagonals[py*w+px]==TL_BR_DIAGONAL))              <<3;
//                psd->adjacent_sample_valid|=((px>0)&&(py<(h-1))&&(vd->diagonals[py*w+px+w]==TR_BL_DIAGONAL))        <<5;
//                psd->adjacent_sample_valid|=((px<(w-1))&&(py<(h-1))&&(vd->diagonals[py*w+px+w+1]==TL_BR_DIAGONAL))  <<7;
                psd->adjacent_sample_valid|=((px<(w-1))&&(py>0)     &&((vd->diagonals[py*w+px+1]==TR_BL_DIAGONAL)   ||(vd->diagonals[py*w+px+1]==BOTH_DIAGONAL)))   <<1;
                psd->adjacent_sample_valid|=((px>0)&&(py>0)         &&((vd->diagonals[py*w+px]==TL_BR_DIAGONAL)     ||(vd->diagonals[py*w+px]==BOTH_DIAGONAL)))     <<3;
                psd->adjacent_sample_valid|=((px>0)&&(py<(h-1))     &&((vd->diagonals[py*w+px+w]==TR_BL_DIAGONAL)   ||(vd->diagonals[py*w+px+w]==BOTH_DIAGONAL)))   <<5;
                psd->adjacent_sample_valid|=((px<(w-1))&&(py<(h-1)) &&((vd->diagonals[py*w+px+w+1]==TL_BR_DIAGONAL) ||(vd->diagonals[py*w+px+w+1]==BOTH_DIAGONAL))) <<7;
            }
        }


        relinquish_all_clusters_and_adjacents(first_cluster,&first_available_cluster,&first_available_adjacent);
        first_cluster=NULL;


        cluster_count=0;


        for(i=0;i<pixel_count;i++)
        {
            psd=pixels+i;

            if(psd->cluster==NULL)
            {
                cluster_count++;
                pc = get_pixel_cluster(&first_available_cluster,&first_cluster);

                psd_same_cluster_stack=psd;

                psd->cluster= &dummy_cluster;/// dummy_clusteracts as identifier to stop double adds
                psd->next=NULL;

                auto_combine_identifier=psd->auto_combine_identifier;

                while(psd_same_cluster_stack)
                {
                    psd=psd_same_cluster_stack;
                    psd_same_cluster_stack=psd_same_cluster_stack->next;

                    add_pixel_to_cluster(pc,psd,adjacent_offsets);

                    for(j=0;j<adjacent_pixel_samples;j++)
                    {
                        if(psd->adjacent_sample_valid & 1<<j)
                        {
                            ap=psd+adjacent_offsets[j];

                            if((ap->cluster==NULL)&&(auto_combine_identifier == ap->auto_combine_identifier))
                            {
                                ap->cluster= &dummy_cluster;/// dummy_cluster acts as identifier to stop double adds
                                ap->next=psd_same_cluster_stack;
                                psd_same_cluster_stack= ap;
                            }
                            else if((ap->cluster!=pc)&&(ap->cluster!= &dummy_cluster)) border_stack[border_stack_size++]=(cluster_border_pixel){.pc=pc,.psd=ap};//,.adjacency_type=test_stack[test_stack_size].type
                        }
                    }
                }

                recalculate_cluster_colour(pc);
            }
        }

        while(border_stack_size)
        {
            border_stack_size--;
            add_adjacent_to_cluster(border_stack[border_stack_size].pc,border_stack[border_stack_size].psd->cluster,&first_available_adjacent);
        }

        new_cluster_count=collapse_clusters(&first_cluster,cluster_count,&first_available_adjacent,&first_available_cluster,&changes,&change_space,combination_threshold,min_valid_cluster_contribution_factor,adjacent_offsets);

        calculate_diagonals(pixels,vd->diagonals,unresolved_diagonals,w,h);
        #warning check that above actually alters previously set values for diagonals when no clusters were split, IT SHOULDN'T !

//        printf("::: %u %u\n",new_cluster_count,cluster_count);
    }
    while(new_cluster_count!=cluster_count);

    printf("diagonal calculation time: %u\n\n",SDL_GetTicks()-t);

    printf("Final cluster count: %u\n",cluster_count);


    remove_all_faces_from_image(vd);///setup for new faces to be created

    if(vd->first_face)exit(17);

    for(i=0;i<pixel_count;i++)
    {
        pc=pixels[i].cluster;

        if(pc->f==NULL)
        {
            pc->f=create_face(vd);
            pc->f->colour=pc->colour;
        }

        vd->clusterised[i]=pc->colour;
        vd->pixel_faces[i]=pc->f;
    }

    ///printf("%d %d %d - %f %f %f\n",pixels[0].cluster->ra>>PREPROCESSING_MAGNITUDE,pixels[0].cluster->ga>>PREPROCESSING_MAGNITUDE,pixels[0].cluster->ba>>PREPROCESSING_MAGNITUDE , pixels[0].cluster->cie_l,pixels[0].cluster->cie_a,pixels[0].cluster->cie_b);

    upload_modified_image_colour(vd,vd->clusterised,VECTORIZER_RENDER_MODE_CLUSTERISED);

    relinquish_all_clusters_and_adjacents(first_cluster,&first_available_cluster,&first_available_adjacent);

    while((adj=first_available_adjacent))
    {
        first_available_adjacent=first_available_adjacent->parent;
        free(adj);
    }

    while((pc=first_available_cluster))
    {
        first_available_cluster=first_available_cluster->next;
        free(pc);
    }

    free(pixels);
    free(border_stack);
    free(unresolved_diagonals);
    free(changes);
}





