#include "kernel/types.h"
#include "user/user.h"

#define MAXREF 100
#define MAXFRAMES 10

int refs[MAXREF];
int nrefs;

void
print_frames(int frames[], int nf)
{
  printf("[ ");
  for(int i = 0; i < nf; i++){
    if(frames[i] == -1)
      printf("- ");
    else
      printf("%d ", frames[i]);
  }
  printf("]");
}

int
fifo(int ref[], int n, int nf, int verbose)
{
  int frames[MAXFRAMES];
  int next = 0;
  int faults = 0;

  for(int i = 0; i < nf; i++)
    frames[i] = -1;

  if(verbose)
    printf("\n--- FIFO (%d frames) ---\n", nf);

  for(int i = 0; i < n; i++){
    int hit = 0;

    for(int j = 0; j < nf; j++){
      if(frames[j] == ref[i]){
        hit = 1;
        break;
      }
    }

    if(!hit){
      frames[next] = ref[i];
      next = (next + 1) % nf;
      faults++;
    }

    if(verbose){
      printf("Reference %d: ", ref[i]);
      print_frames(frames, nf);
      printf("  %s\n", hit ? "HIT" : "FAULT");
    }
  }

  if(verbose)
    printf("FIFO page faults = %d, hits = %d\n", faults, n - faults);

  return faults;
}

int
lru(int ref[], int n, int nf, int verbose)
{
  int frames[MAXFRAMES];
  int last_used[MAXFRAMES];
  int faults = 0;

  for(int i = 0; i < nf; i++){
    frames[i] = -1;
    last_used[i] = -1;
  }

  if(verbose)
    printf("\n--- LRU (%d frames) ---\n", nf);

  for(int i = 0; i < n; i++){
    int hit = 0;
    int pos = -1;

    for(int j = 0; j < nf; j++){
      if(frames[j] == ref[i]){
        hit = 1;
        pos = j;
        break;
      }
    }

    if(hit){
      last_used[pos] = i;
    } else {
      int victim = -1;

      for(int j = 0; j < nf; j++){
        if(frames[j] == -1){
          victim = j;
          break;
        }
      }

      if(victim == -1){
        victim = 0;
        for(int j = 1; j < nf; j++){
          if(last_used[j] < last_used[victim])
            victim = j;
        }
      }

      frames[victim] = ref[i];
      last_used[victim] = i;
      faults++;
    }

    if(verbose){
      printf("Reference %d: ", ref[i]);
      print_frames(frames, nf);
      printf("  %s\n", hit ? "HIT" : "FAULT");
    }
  }

  if(verbose)
    printf("LRU page faults = %d, hits = %d\n", faults, n - faults);

  return faults;
}

void
print_summary(int ref[], int n, int nf)
{
  int ff = fifo(ref, n, nf, 0);
  int lf = lru(ref, n, nf, 0);
  int fh = n - ff;
  int lh = n - lf;
printf("%d | %d | %d%% | %d | %d%%\n",
       nf, ff, (fh * 100) / n, lf, (lh * 100) / n);
}

int
main(int argc, char *argv[])
{
  int default_ref[] = {
    7, 0, 1, 2, 0, 3, 0, 4, 2, 3,
    0, 3, 2, 1, 2, 0, 1, 7, 0, 1
  };
  int nf;

  if(argc >= 3){
    // Custom input: pagerepl <frames> <reference1> <reference2> ...
    nf = atoi(argv[1]);

    if(nf < 1 || nf > MAXFRAMES || argc - 2 > MAXREF){
      printf("Usage: pagerepl <1-%d frames> <references...>\n",
             MAXFRAMES);
      exit(1);
    }

    nrefs = argc - 2;
    for(int i = 0; i < nrefs; i++)
      refs[i] = atoi(argv[i + 2]);

    fifo(refs, nrefs, nf, 1);
    lru(refs, nrefs, nf, 1);
    exit(0);
  }

  nrefs = sizeof(default_ref) / sizeof(default_ref[0]);
  for(int i = 0; i < nrefs; i++)
    refs[i] = default_ref[i];

  printf("Reference string: ");
  for(int i = 0; i < nrefs; i++)
    printf("%d ", refs[i]);
  printf("\n");

  fifo(refs, nrefs, 3, 1);
  lru(refs, nrefs, 3, 1);

  printf("\n--- Comparison for 3, 4 and 5 frames ---\n");
  printf(" Frames | FIFO faults | FIFO hits | LRU faults | LRU hits\n");
  printf("--------+-------------+-----------+------------+---------\n");

  for(int f = 3; f <= 5; f++)
    print_summary(refs, nrefs, f);

  exit(0);
}