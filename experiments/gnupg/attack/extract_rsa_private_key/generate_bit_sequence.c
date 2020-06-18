#include<stdio.h>
/*
* Determine the bit sequence from a given sequence of R,S and M
*/
int main(){
    int choice;
    printf("1 for true_seq, 0 for spy\n");
    scanf("%d",&choice);
    FILE *file;
    if(choice)
        file = fopen("true_seq.txt","r");
    else
        file = fopen("result.txt","r");
    char c[100000];
    char bit_seq[100000];
    int bit_seq_ctr = 0;
    int cnt = 0;
    while(fscanf(file,"%c",&c[cnt])==1){
        cnt++;
    }
    c[cnt] = '\0';
    int srm_toggle = 0;
    for(int i=0;i<cnt;i++){
        if(srm_toggle){
            if(c[i] == 'R'){
                bit_seq[bit_seq_ctr] = '1';
                bit_seq_ctr++;
                srm_toggle = 0;
            }
            else if(c[i] == 'M'){
                continue;
            }
        }
        else if(c[i]== 'S' && c[i+1] == 'R' && c[i+2]=='M'){
            if(c[i+3]=='R'){
                bit_seq[bit_seq_ctr] = '1';
                bit_seq_ctr++;
                i += 3;
            }
            else if(c[i+3]=='M'){
                i += 3;
                srm_toggle = 1;
                continue;
            }
        }
        else if(c[i] == 'S' && c[i+1] == 'R' && c[i+2] != 'M'){
            bit_seq[bit_seq_ctr] = '0';
            bit_seq_ctr++;
            i += 2;
        }
    }
    bit_seq[bit_seq_ctr] = '\0';
    cnt = 0;
    while(bit_seq[cnt]){
        printf("%c",bit_seq[cnt]);
        cnt++;
    }
    printf("\n");
    return 0;
}