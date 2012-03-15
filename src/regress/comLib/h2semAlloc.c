#include <portLib.h>
#include <h2semLib.h>

#define POCOREGRESS_NSEMS  (2 * MAX_SEM + 1)

H2SEM_ID pocoregress_tab[POCOREGRESS_NSEMS + 1];

int
pocoregress_init(void)
{
	int i;

	for (i = 0; i < POCOREGRESS_NSEMS; i++) {
		pocoregress_tab[i] = h2semAlloc(H2SEM_EXCL);
		if (pocoregress_tab[i] == ERROR) {
			return 2;
		}
	}
	printf("allocated %d semaphores\n", POCOREGRESS_NSEMS);
	for (i = 0; i < POCOREGRESS_NSEMS; i++) {
		h2semDelete(pocoregress_tab[i]);
	}
	printf("deleted %d semaphores\n", POCOREGRESS_NSEMS);
	return OK;
}
