#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    FILE * fp, * fp2;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    unsigned int spy_lines = 0;

    fp = fopen("abcd_exec_time", "r");
    fp2 = fopen("spy_exec_time", "r");
    unsigned long actual[300], spy[10000];
    char *p;
    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1 && i<252) {
        actual[i] = strtoull(line,NULL,10);
        i++;
    }
    fclose(fp);
    line = NULL;
    len = 0;
    while ((read = getline(&line, &len, fp2)) != -1) {
        spy[spy_lines] = strtoull(line,NULL,10);
	spy_lines++;
    }
    fclose(fp2);
    for(i=0;i<252;i++){
        for(int j=0;j<spy_lines;j++){
            if((spy[j]-actual[i]<=150000)||(actual[i]-spy[j]<=150000)){
                actual[i] = 0;
                spy[j] = 0;
            }
        }
    }
    int false_neg = 0, false_pos = 0, true_pos = 0, true_neg = 0;
    for(i=0;i<252;i++){
        if(actual[i])
            false_neg++;
	else
	    true_pos++;
    }
    for(i=0;i<spy_lines;i++){
        if(spy[i])
            false_pos++;
    }
    false_pos = ((false_pos>772)?772:false_pos);
    true_neg = 772 - false_pos;
    printf("Number of true negatives:\t%d\n",true_neg);
    printf("Number of false negatives:\t%d\n",false_neg);
    printf("Number of true positives:\t%d\n",true_pos);
    printf("Number of false positives:\t%d\n",false_pos);
    printf("Number of entries in spy:\t%d\n",spy_lines);
    if (line)
        free(line);
    exit(EXIT_SUCCESS);
}
