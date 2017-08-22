
#include "cfg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#define atoll(x) _atoi64(x)
#endif

struct _item_node
{
	char name[32];
	char value[256];
	struct _item_node* next;
};

struct _group_node
{
	char name[32];
	struct _item_node* item;
	struct _group_node* next;
};

char* trim(char* str)
{
	char* p = NULL;
	unsigned int len = (unsigned int)strlen(str);
	unsigned int i = 0;
	while(isspace(str[i]))
	{
		str[i] = 0;
		i++;
	}

	p = str+i;

	i = len - 1;
	while(isspace(str[i]))
	{
		str[i] = 0;
		i--;
	}

	return p;
}

const char*
	find_cfg_str(const char* group, const char* key, struct _group_node* grp)
{
	const char* val = NULL;
	const struct _item_node* item = NULL;

	if(group == NULL||strlen(group)==0)
		group = "unnamed";

	while(grp)
	{
		if(strcmp(group, grp->name) == 0)
		{
			break;
		}
		grp = grp->next;
	}

	if(grp == NULL)
		return NULL;

	item = grp->item;
	while(item)
	{
		if(strcmp(key, item->name) == 0)
		{
			val = item->value;
			break;
		}
		item = item->next;
	}

	return val;
}

static struct _group_node*
	load_config(const char* cfg_file)
{
	struct _group_node* root = NULL;
	struct _group_node* cur_grp = NULL;
	FILE* fp = NULL;
	char line_buf[1024] = "";

	if(cfg_file == NULL)
	{
		return NULL;
	}

	fp = fopen(cfg_file, "r");
	if (fp == NULL)
	{
		return NULL;
	}

	while(fgets(line_buf, 1024, fp))
	{
		int len = 0;
		char* line = trim(line_buf);
		if(line == NULL)
			continue;

		len = (int)strlen(line);
		if(len>2)
		{
			if(line[0] == '[' && line[len-1] == ']')
			{
				/*a group*/
				struct _group_node* grp = (struct _group_node*)malloc(sizeof(struct _group_node));
				grp->item = NULL;
				grp->next = NULL;
				strncpy(grp->name, line+1, len-2);
				grp->name[len-2] = 0;
				if(cur_grp == NULL)
					root = cur_grp = grp;
				else
				{
					cur_grp->next = grp;
					cur_grp = grp;
				}
				continue;
			}
		}

		{
			struct _item_node* item = NULL;
			char* key = line;
			char* val = strchr(line,'=');
			if (line[0] == '#' || val == NULL)
			{
				continue;
			}

			*val = 0;
			val = val+1;
			key = trim(key);
			val = trim(val);

			if(key == NULL || strlen(key) == 0)
				continue;
			if (val == NULL||strlen(val)==0)
				continue;

			/*key=value*/
			if(cur_grp == NULL)
			{
				struct _group_node* grp = (struct _group_node*)malloc(sizeof(struct _group_node));
				grp->item = NULL;
				grp->next = NULL;
				strcpy(grp->name, "unnamed");
				grp->name[7] = 0;
				root = cur_grp = grp;
			}

			item = (struct _item_node*)malloc(sizeof(struct _item_node));
			item->next = NULL;
			strcpy(item->name, key);
			strcpy(item->value, val);

			if(cur_grp->item == NULL)
				cur_grp->item = item;
			else
			{
				struct _item_node* last_item = cur_grp->item;
				while(last_item->next)
					last_item = last_item->next;
				last_item->next = item;
			}
		}
	}/*while*/

	fclose(fp);

	return root;
}

void free_cfg(struct _group_node* grp)
{
	struct _group_node* next_grp = NULL;
	struct _item_node* next_item = NULL;
	while(grp)
	{
		while(grp->item)
		{
			next_item = grp->item->next;
			free(grp->item);
			grp->item = next_item;
		}

		next_grp = grp->next;
		free(grp);
		grp = next_grp;
	}
}

void show_cfg(struct _group_node* grp)
{
	struct _group_node* next_grp = grp;
	struct _item_node* next_item = NULL;
	while(next_grp)
	{
		printf("[%s]\n", next_grp->name);

		next_item = next_grp->item;
		while(next_item)
		{
			printf("\t%s\t= %s\n", next_item->name, next_item->value);
			next_item = next_item->next;
		}

		next_grp = next_grp->next;
	}
}

CFG_HANDLE cfg_create(const char* cf)
{
	struct _group_node* grp = NULL;
	grp = load_config(cf);
#ifdef _DEBUG
	show_cfg(grp);
#endif
	return (CFG_HANDLE)grp;
}

const char* cfg_get_str(const char* grp_name, const char* key, CFG_HANDLE cfgh)
{
	struct _group_node* grp = (struct _group_node*)cfgh;
	return find_cfg_str(grp_name, key, grp);
}

int cfg_get_int(const char* grp, const char* key, int def, CFG_HANDLE cfgh)
{
	const char* val = cfg_get_str(grp, key, cfgh);
	if(val)
	{
		return atoi(val);
	}
	return def;
}

long cfg_get_long(const char* grp, const char* key, long def, CFG_HANDLE cfgh)
{
	const char* val = cfg_get_str(grp, key, cfgh);
	if(val)
	{
		return atol(val);
	}
	return def;
}

long long cfg_get_longlong(const char* grp, const char* key, long long def, CFG_HANDLE cfgh)
{
	const char* val = cfg_get_str(grp, key, cfgh);
	if(val)
	{
		return atoll(val);
	}
	return def;
}

float cfg_get_float(const char* grp, const char* key, float def, CFG_HANDLE cfgh)
{
	const char* val = cfg_get_str(grp, key, cfgh);
	if(val)
	{
		return (float)atof(val);
	}
	return def;
}

double cfg_get_double(const char* grp, const char* key, double def, CFG_HANDLE cfgh)
{
	const char* val = cfg_get_str(grp, key, cfgh);
	if(val)
	{
		return atof(val);
	}
	return def;
}

void cfg_release(CFG_HANDLE cfgh)
{
	struct _group_node* grp = (struct _group_node*)cfgh;
	free_cfg(grp);
}

unsigned int get_cfg_str_file(const char* grp_name, const char* key, const char* default_val, char* buf, unsigned int buf_len, const char* cf)
{
	unsigned int ret = 0;
	const char* p = NULL;
	CFG_HANDLE h = NULL;

	h = cfg_create(cf);

	if(!h)
		p = cfg_get_str(grp_name, key, h);

	if(p == NULL)
		p = default_val;

	if(p != NULL)
	{
		ret = (unsigned int)strlen(p);
		if(ret > buf_len)
			ret = buf_len;
		strncpy(buf, p, ret);
	}

	if(!h)
		cfg_release(h);

	return ret;
}
