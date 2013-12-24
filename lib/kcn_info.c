#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_str.h"
#include "kcn_info.h"

struct kcn_loc {
	char *kl_uri;
	size_t kl_score;
};

struct kcn_info {
	enum kcn_loc_type ki_loctype;
	size_t ki_maxnlocs;
	char ki_dbname[KCN_INFO_DBNAMESTRLEN];
	char ki_country[KCN_INFO_COUNTRYSTRLEN];
	char ki_userip[KCN_INET_ADDRSTRLEN];
	size_t ki_nlocs;
	struct kcn_loc ki_locs[1];
};

static void kcn_info_db_unset(struct kcn_info *);

struct kcn_info *
kcn_info_new(enum kcn_loc_type loctype, size_t maxnlocs)
{
	struct kcn_info *ki;

	assert(maxnlocs > 0);
	ki = malloc(offsetof(struct kcn_info, ki_locs[maxnlocs]));
	if (ki == NULL)
		return NULL;
	ki->ki_loctype = loctype;
	ki->ki_maxnlocs = maxnlocs;
	ki->ki_dbname[0] = '\0';
	ki->ki_country[0] = '\0';
	ki->ki_userip[0] = '\0';
	ki->ki_nlocs = 0;
	return ki;
}

void
kcn_info_destroy(struct kcn_info *ki)
{
	size_t i;

	kcn_info_db_unset(ki);
	for (i = 0; i < ki->ki_nlocs; i++)
		free(ki->ki_locs[i].kl_uri);
	free(ki);
}

enum kcn_loc_type
kcn_info_loc_type(const struct kcn_info *ki)
{

	return ki->ki_loctype;
}

size_t 
kcn_info_maxnlocs(const struct kcn_info *ki)
{

	return ki->ki_maxnlocs;
}

void
kcn_info_db_set(struct kcn_info *ki, const char *dbname)
{

	kcn_info_db_unset(ki);
	if (dbname == NULL)
		return;
	strlcpy(ki->ki_dbname, dbname, sizeof(ki->ki_dbname));
}

static void
kcn_info_db_unset(struct kcn_info *ki)
{

	ki->ki_dbname[0] = '\0';
}

const char *
kcn_info_db(const struct kcn_info *ki)
{

	return ki->ki_dbname[0] == '\0' ? NULL : ki->ki_dbname;
}

void
kcn_info_country_set(struct kcn_info *ki, const char *country)
{

	if (country == NULL)
		return;
	assert(country[0] != '\0');
	ki->ki_country[0] = country[0];
	ki->ki_country[1] = country[1];
	ki->ki_country[2] = '\0';
}

const char *
kcn_info_country(const struct kcn_info *ki)
{

	if (ki->ki_country[0] == '\0')
		return NULL;
	else
		return ki->ki_country;
}

void
kcn_info_userip_set(struct kcn_info *ki, const char *userip)
{

	if (userip == NULL)
		return;
	strlcpy(ki->ki_userip, userip, sizeof(ki->ki_userip));
}

const char *
kcn_info_userip(const struct kcn_info *ki)
{

	if (ki->ki_userip[0] == '\0')
		return NULL;
	else
		return ki->ki_userip;
}

size_t
kcn_info_nlocs(const struct kcn_info *ki)
{

	return ki->ki_nlocs;
}

const char *
kcn_info_loc(const struct kcn_info *ki, size_t idx)
{

	if (idx >= ki->ki_nlocs)
		return NULL;
	return ki->ki_locs[idx].kl_uri;
}

bool
kcn_info_loc_add(struct kcn_info *ki, const char *locstr, size_t locstrlen,
    size_t score)
{
	struct kcn_loc *kl = &ki->ki_locs[ki->ki_nlocs];

	assert(ki->ki_nlocs < ki->ki_maxnlocs);
	kl->kl_uri = kcn_str_dup(locstr, locstrlen);
	if (kl->kl_uri == NULL)
		return false;
	kl->kl_score = score;
	++ki->ki_nlocs;
	return true;
}
