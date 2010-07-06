
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lo/lo.h>

#include "mapper_internal.h"
#include "types_internal.h"
#include "operations.h"
#include "expression.h"
#include <mapper/mapper.h>

void get_expr_Tree (Tree *T/**/,char *expr/**/);

mapper_router mapper_router_new(const char *host, int port, char *name)
{
    char str[16];
    mapper_router router = calloc(1,sizeof(struct _mapper_router));
    sprintf(str, "%d", port);
    router->addr = lo_address_new(host, str);
	router->target_name=strdup(name);

    if (!router->addr) {
        mapper_router_free(router);
        return 0;
    }
    return router;
}

void mapper_router_free(mapper_router router)
{
    if (router) {
        if (router->addr)
            lo_address_free(router->addr);
        if (router->mappings)
        {
            mapper_signal_mapping sm = router->mappings;
            while (sm)
            {
                mapper_signal_mapping tmp = sm->next;
                if (sm->mapping) {

                    mapper_mapping m = sm->mapping;
                    while (m) {
                        mapper_mapping tmp = m->next;
                        if (tmp->name)
                            free(tmp->name);
                        free(m);
                        m = tmp;
                     }
                }
                free(sm);
                sm = tmp;
            }
        }
        free(router);
    }
}

void mapper_router_receive_signal(mapper_router router, mapper_signal sig,
                                  mapper_signal_value_t *value)
{

    // find this signal in list of mappings
    mapper_signal_mapping sm = router->mappings;
    while (sm && sm->signal != sig)
        sm = sm->next;

    // exit without failure if signal is not mapped
    if (!sm)
		{
			return;
		}


    // for each mapping, construct a mapped signal and send it
    mapper_mapping m = sm->mapping;
	int c=1;
	int i=0;
	char *name;
    while (m)
    {
        struct _mapper_signal signal = *sig; 
        c=1;
		i=0;

		/*signal.name = m->name;*/  
		printf("RECEIVE : MAPPING NAME 1 = %s\n", m->name);  
		/**************************************************************************************************************************/
		/*int c=1;
		int i=0;
		char *name;*/
		name=strdup(m->name);
		while ((/*signal.*/name)[c]!='/' && c<strlen(/*signal.*/name))
			c++;
		if (c<strlen(/*signal.*/name) && c>1)
			{
				while ((/*signal.*/name)[c+i]!='\0')
					{				
						name[i]=(/*signal.*/name)[c+i];
						i++;
						printf("c+i= %d name en construction : %s\n", c+i, name);
					}
				/*name[c+i]='\0';*/
				printf("c+i : %d name fin construction : %s\n",c+i, name);
				signal.name=strndup(name,i);
				free(name);
			}
		/**************************************************************************************************************************/
		printf("RECEIVE : SIGNAL NAME 2 = %s\n", signal.name);       
		mapper_signal_value_t v;
        mapper_mapping_perform(m, value, &v);
        mapper_router_send_signal(router, &signal, &v);
        m = m->next;
    }
}

void mapper_router_send_signal(mapper_router router, mapper_signal sig,
                               mapper_signal_value_t *value)
{

    int i;
    lo_message m;
    if (!router->addr) 
		{	
			return;
		}

    m = lo_message_new();
    if (!m) return;

    for (i=0; i<sig->length; i++) {
    mval_add_to_message(m, sig, &value[i]);
	printf("%f bien ajouté au message\n", value->f);}

	int send;
    /***********************************************************/send=lo_send_message(router->addr, sig->name, m);/**********************************************************/

	printf("%s\n\n",send==-1?"ECHEC ENVOI MESSAGE":"SUCCES ENVOI MESSAGE");
    lo_message_free(m);
    return;
}

void mapper_router_add_mapping(mapper_router router, mapper_signal sig,
                               mapper_mapping mapping)
{
    // find signal in signal mapping list
    mapper_signal_mapping sm = router->mappings;
    while (sm && sm->signal != sig)
        sm = sm->next;

    // if not found, create a new list entry
    if (!sm) {
        sm = (mapper_signal_mapping)
            calloc(1,sizeof(struct _mapper_signal_mapping));
        sm->signal = sig;
        sm->next = router->mappings;
        router->mappings = sm;
    }

    // add new mapping to this signal's list
    mapping->next = sm->mapping;
    sm->mapping = mapping;
}

/*****************************************************************************************************************************************/

void mapper_router_remove_mapping(mapper_router router, mapper_signal sig, char *dest_name)
{

    mapper_signal_mapping *sm = &router->mappings;
    while (*sm && (*sm)->signal != sig)
        sm = &(*sm)->next;

    if (!sm) return;

	mapper_mapping *m=&(*sm)->mapping;
	while (*m)
		{
			printf("Mapping a supprimer : %s, mapping comparé %s\n",(*m)->name,dest_name);
			if (strcmp((*m)->name,dest_name)==0)
				{
					printf("OK !\n");
					*m=(*m)->next;
					break;
				}
			m = &(*m)->next;
		}

}


	/*mapper_router *r = &md->routers;
    while (*r) 
		{
	        if (*r == rt) 
				{
            		*r = rt->next;
            		break;
        		}
        	r = &(*r)->next;
    	}*/




void mapper_router_add_direct_mapping(mapper_router router, mapper_signal sig,
                                      const char *name, float src_min, float src_max, float dest_min, float dest_max)
{
    mapper_mapping mapping =
        calloc(1,sizeof(struct _mapper_mapping));

    mapping->type=BYPASS;
    mapping->name = strdup(name);
	mapping->expression = strdup("y=x");
	mapping->range[0]=src_min;
	mapping->range[1]=src_max;
	mapping->range[2]=dest_min;
	mapping->range[3]=dest_max;

    mapper_router_add_mapping(router, sig, mapping);
}

void mapper_router_add_linear_mapping(mapper_router router, mapper_signal sig,
                                      const char *name, /*mapper_signal_value_t scale,*/ char *expr, float src_min, float src_max, float dest_min, float dest_max)
{
    mapper_mapping mapping =
        calloc(1,sizeof(struct _mapper_mapping));

    mapping->type=LINEAR;
    mapping->name = strdup(name);

	mapping->expression = strdup(expr);
	mapping->range[0]=src_min;
	mapping->range[1]=src_max;
	mapping->range[2]=dest_min;
	mapping->range[3]=dest_max;

    Tree *T=NewTree();
    get_expr_Tree(T, expr);
    mapping->expr_tree=T;

		

    /*mapping->coef_input[0] = scale.f;
    mapping->order_input = 1;

	mapping->expression = strdup(expression);
	mapping->range[0]=src_min;
	mapping->range[1]=src_max;
	mapping->range[2]=dest_min;
	mapping->range[3]=dest_max;*/


    mapper_router_add_mapping(router, sig, mapping);
}

void mapper_router_add_expression_mapping(mapper_router router, mapper_signal sig,
                                      const char *name, char *expr, float src_min, float src_max, float dest_min, float dest_max)
{
    mapper_mapping mapping =
        calloc(1,sizeof(struct _mapper_mapping));

    mapping->type=EXPRESSION;
    mapping->name = strdup(name);
	mapping->expression = strdup(expr);
	mapping->range[0]=src_min;
	mapping->range[1]=src_max;
	mapping->range[2]=dest_min;
	mapping->range[3]=dest_max;

    Tree *T=NewTree();
    get_expr_Tree(T, expr);
    mapping->expr_tree=T;

    mapper_router_add_mapping(router, sig, mapping);
}
