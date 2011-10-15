/*
Program to find repeated digits in pi

Method is to store digits of pi in a long string, and populate a set with indices to positions in this string.
We implement the set via a wide array to lookup from the value of the first K digits to a singly linked list to pointers to increasing values within the string.

We store a batch of up to B entries in the list. (Requires B+N-1 digits in string, N for first string, and B-1 extras for remainder)
If no hit is found in this time we carry on searching until we get a match without adding new entries to the list.
We then go back and repeat the search to see if a smaller value may be found. 

for FILE in *.zip; do unzip $FILE; done

gcc -o pi_repeat.exe -O3 -m64 pi_repeat.c

Seems a lot slower with *5, perhaps doesn't fully outweigh the advantage of fewer passes?

Perhaps an alternative hash would result in better cache behaviour? (Opposite to normal desired effect of hash)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct pi_gen_s
{
	FILE *fid;
	int filenum;
	int line;  // line in the file
	int group; // group within a line
	int pos;  // digit within a group
	char text[256];
	char *cpos;
} PI_GEN_T;

typedef struct node_s
{
	int next; // 0 means none, or index into node array
	int pos;  // index of first digit in captured string
} NODE_T;

NODE_T *g_nodes=NULL;
int g_num_nodes=1;
int *g_set=NULL; // index into node array.
char *g_digits;
int g_hashlen=0;

/* K10=10**K */
void alloc_storage(int N,int B, int K10)
{
	g_nodes=calloc(sizeof(NODE_T),B+1); // 0 node means no node present
	assert(g_nodes);
	g_set=calloc(sizeof(int),K10);
	assert(g_set);
	g_digits=malloc(N+B);
	assert(g_digits);
	g_hashlen=K10;
}

void set_clear(int K10)
{
	g_num_nodes=1;
	memset(g_set,0,sizeof(int)*g_hashlen);
}

/*
Key is the index into the array made from bottom K digits of the number to test.
digit points to the first character of the number to test.
Returns 1 if matches, or 0 otherwise.

If insert then we will add a new node if we have a miss
*/
int set_test(int key,char *digit,int N,int K,int insert)
{
	int *p=&g_set[key];
	int i;
	//printf("Testing %d\n",key);
	while(*p)
	{
		NODE_T *n=g_nodes+*p;
		// Check that this number is less than ours, from key we know last K digits match
		for(i=0;i<N-K;i++)
		{
			int diff=g_digits[n->pos+i]-digit[i];
			if (diff>0) goto no_match;
			if (diff<0) goto try_next; // No match here, but a later one might
		}
		// Found a match!
		return 1;
try_next:
		p=&n->next;
	}
no_match:
	if (!insert)
		return 0;
	g_nodes[g_num_nodes].pos=digit-g_digits;
	g_nodes[g_num_nodes].next=*p;
	*p=g_num_nodes;
	g_num_nodes++;
	return 0;
}

/* Returns the next digit of pi */
int pi_gen(PI_GEN_T *p)
{
	char c;
	if (p->cpos==0)
	{
		if (p->line==0)
		{
			char name[256];
			//sprintf(name,"/cygdrive/c/data/pi/pi-%.04d.txt",p->filenum+1);
			sprintf(name,"c:\\data\\pi\\pi-%.04d.txt",p->filenum+1);
			printf("Opening %s\n",name);
			fflush(stdout);
			p->fid=fopen(name,"r");
			assert(p->fid);
		}
		fgets(p->text,256,p->fid);
		p->cpos=p->text;
	}
	c=*p->cpos++;
	if (c==' ')
	{
		if (p->pos==100)
		{
			p->line+=1;
			p->cpos=0;
			p->pos=0;
			if (p->line==1000000)
			{
				fclose(p->fid);
				p->line=0;
				p->filenum++;
			}
			return pi_gen(p);
		}
		c=*p->cpos++;
	}
	p->pos++; //p->pos is number of digits read
	return c-'0';
}

/*
Return a generator of digits of pi from given position

Takes 1.47 seconds to read 100million digits (or 0.144 if cached)
*/
PI_GEN_T *make_pi_gen(long long pos)
{
	PI_GEN_T *p=(PI_GEN_T *)calloc(sizeof(PI_GEN_T),1);
	while (pos>=100000000)
	{
		p->filenum++;
		pos-=100000000;
	}
	while (pos>0)
	{
	  pi_gen(p);
	  pos--;
	}
	return p;
}

int main(int argc, char *argv[])
{
	unsigned long long t=0;
	unsigned long long start=0;
	unsigned long long best=0;
	int N;
	int i;
	PI_GEN_T *p;
	int K10=1;
	int B=100000000*5; // Can increase this to use more memory , needs to fit inside 32bit signed int
	int K=8; // 8 digits should roughly match B to get a low hit ratio, used to set the size of the hash table, must be less than N
	int key=0;
	int j;
	
	if (argc!=2)
	{
		fprintf(stderr,"Usage: pi N\nFinds number of decimal digits of pi needed to have a repeated N digit string of digits.\nRelies on digits of pi being present in c:/data/pi/pi-0001.txt files.\n");
		return -1;
	}
	N=atoi(argv[1]);
	printf("Searching for a repeated string of %d digits in pi...(%d,%d,%d,%d)\n",N,sizeof(NODE_T),sizeof(NODE_T*),sizeof(int),sizeof(short));
	fflush(stdout);
	K10=1;
	for(i=0;i<K;i++)
	{
		K10*=10;
	}
	alloc_storage(N,B,K10);
	
	assert(K<N);
	
	while((best==0 || start<best) && (start<=15000000000LL))
	{
		unsigned long long dist;
		set_clear(K10);
		char test_digits[N];
		//printf("Restarting at %llu\n",start);
		p=make_pi_gen(start);
		//printf("Scanning\n",start);
		for(i=0;i<N-1;i++)
		{
			int v=pi_gen(p);
			g_digits[i]=v;
			key=(key*10+v)%K10;
		}
		for(i=0;i<B;i++)
		{
			int v=pi_gen(p);
			g_digits[N-1+i]=v;
			key=(key*10+v)%K10;
			if (set_test(key,g_digits+i,N,K,1))
			{
				// Found match while making set, so must be the best answer
				printf("Found match %d at position %llu (%llu total digits):",N,i+start,i+start+N);
				fflush(stdout);
				for(j=0;j<N;j++) printf("%d",g_digits[i+j]);
				printf("\n");
				return 0;
			}
		}
		/* Now keep searching until find a match or reach the end of our data set...*/
		// Copy over the digits of the last test performed (at i before the increment, so i-1 after the increment of i)
		for(j=0;j<N;j++)
		{
			test_digits[j]=g_digits[i+j-1];
		}
		dist=start+i;
		//printf("Trying remainder at %llu %llu %llu\n",dist,start,best);
		while((best==0 || dist<best) && (dist<=15000000000LL))
		{
			int v=pi_gen(p);
			for(j=0;j<N-1;j++)
			{
				test_digits[j]=test_digits[j+1];
			}
			test_digits[N-1]=v;
			key=(key*10+v)%K10;
			if (set_test(key,test_digits,N,K,0))
			{
				// Found match while making set, so must be the best answer
				best=dist;
				printf("Found possible match %d at position %llu (%llu total digits):",N,dist,dist+N);
				fflush(stdout);
				for(j=0;j<N;j++) printf("%d",test_digits[j]);
				printf("\n");
				break;
			}
			dist+=1;
		}
		start+=B; // May find a better match later on
		
	}
	printf("Best=%llu\n",best);
	fflush(stdout);
	return 0;
}

// 16: Found possible match at position 129440087 (129440103 total digits):8230687217052243
// Found possible match 17 at position  262527965 (262527982 total digits):93415455347042966 (3minutes)
// (Found possible match 18 at position10382281876 (10382281894 total digits):492496284054724445)
// Found possible match 18 at position 1982424642 (1982424660 total digits):013724950651727463
// (Found possible match 19 at position 9572305981 (9572306000 total digits):8270295137644323301...)
// Found possible match 19 at position 8858170605 (8858170624 total digits):1350168131352524443