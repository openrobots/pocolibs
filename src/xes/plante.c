/**
 ** Essai de faire planter Xes
 **/
#include <vxWorks.h>
#include <stdioLib.h>
#include <errnoLib.h>

#include "posterLib.h"

static char *menu = 
"\n\
Ceci est un menu\n\n1 rien\n2 encore moins\n3 toujours rien\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
10 qwertyuiop\n\
11 asdfghjkl;\n\
12 zxcvbnm,.\n\
13 1234567890\n\
14 fdkgkljdgxcvxcl,m\n\
15 gfgdfg edrt retd fgasd sadas\n\
16 fsdfsdf rrrf sdfsdf sdfesd sfdfsd\n\
17 dsfsdf sdf dsfsd dsfdsfsd ds\n\
18 dfsdfds dsfsdf dssd fdssdfsdsd \n\
99 Fin\n\
\nTapez un numero d'option: ";


planteXes()
{
    int i;
    POSTER_ID poster;

    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    xes_init(NULL);

    posterFind("XESposter", &poster);
    
    while (1) {
	do 
	    printf1(menu);
        while (scanf("%d", &i) != 1);
	printf("\nMerci\n");
	if (i == 99)
	    break;
	posterRead(poster, 0, &i, sizeof(int));
	printf("i: %d bilan: %d\n", i, errnoGet());
    }
    return(OK);
}

mangeCpu()
{
    int i = 0;
    POSTER_ID poster;

    posterCreate("XESposter", 100, &poster);

    while (1) {
	i++;
	posterWrite(poster, 0, &i, sizeof(int));
	taskDelay(5);
    }
}
	
     
planteInit(char *nom)
{
    POSTER_ID posterId;
    
    if (nom == NULL) {
	printf("il faut un nom de machine\n");
	return(ERROR);
    }
    xes_set_host(nom);

    taskSpawn("tCpu", 2, 0x10, 10000, mangeCpu);

    taskDelay(200);

    taskSpawn("tPlante", 20, 0x10, 10000, planteXes);

}
