#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_info.h"

struct kcn_info {
	enum kcn_type ki_type;
	enum kcn_loc_type ki_loctype;
	size_t ki_maxnlocs;
	char ki_country[KCN_INFO_COUNTRYSTRLEN];
	char ki_userip[KCN_INET_ADDRSTRLEN];
	size_t ki_nlocs;
	char *ki_locs[1];
};

struct kcn_info *
kcn_info_new(enum kcn_type type, enum kcn_loc_type loctype, size_t maxnlocs,
    const char *country, const char *userip)
{
	struct kcn_info *ki;

	ki = malloc(offsetof(struct kcn_info, ki_locs[maxnlocs]));
	if (ki == NULL)
		return NULL;
	ki->ki_type = type;
	ki->ki_loctype = loctype;
	ki->ki_maxnlocs = maxnlocs;
	if (country == NULL)
		ki->ki_country[0] = '\0';
	else {
		assert(country[0] != '\0');
		ki->ki_country[0] = country[0];
		ki->ki_country[1] = country[1];
		ki->ki_country[2] = '\0';
	}
	if (userip == NULL)
		ki->ki_userip[0] = '\0';
	else
		strlcpy(ki->ki_userip, userip, sizeof(ki->ki_userip));
	ki->ki_nlocs = 0;
	return ki;
}

void
kcn_info_destroy(struct kcn_info *ki)
{
	size_t i;

	for (i = 0; i < ki->ki_nlocs; i++)
		free(ki->ki_locs[i]);
	free(ki);
}

enum kcn_type
kcn_info_type(const struct kcn_info *ki)
{

	return ki->ki_type;
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

const char *
kcn_info_country(const struct kcn_info *ki)
{

	if (ki->ki_country[0] == '\0')
		return NULL;
	else
		return ki->ki_country;
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
	return ki->ki_locs[idx];
}

bool
kcn_info_loc_add(struct kcn_info *ki, const char *locstr)
{

	assert(ki->ki_nlocs < ki->ki_maxnlocs);
	ki->ki_locs[ki->ki_nlocs] = strdup(locstr);
	if (ki->ki_locs[ki->ki_nlocs] == NULL)
		return false;
	++ki->ki_nlocs;
	return true;
}
