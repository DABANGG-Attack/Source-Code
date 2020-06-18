/**
 * This program is based on the work from Taylor Hornby available here: https://github.com/defuse/flush-reload-attacks
 */

#include <stdio.h>
#include <stdlib.h>

#define REPEAT(A) A;A;A;A;A;A;A;A;A;A;
#define BYPASS_BRANCH_PREDICTION
#define ITERS (3000)

void input_A() {
    int x = 3;
    int y = 4;

    REPEAT(x += y;
                   x *= y)
    REPEAT(x += y;
                   x *= y)
    /*  the exit condition for the loop translates
        to a cmpl assembly instruction, which is spied upon.
        same is applicable to all other input_X functions
     */
    for (long i = 0; i < ITERS; i++) {  
	    x *= y;
        x += y;
        x *= y;
        x *= y;
    }
    x *= y;
    x += y;
    x *= y;
    x *= y;
    //printf("You entered A\n");
    unsigned long time, delta;
    __asm__ __volatile__ ("mfence");
    __asm__ __volatile__ ("lfence");
    __asm__ __volatile__ ("rdtsc" : "=a" (time), "=d" (delta));
    time = (delta<<32) | time;
    printf("%ld\n",time);
    // printf("A\n");
}

void input_B() {
    int x = 3;
    int y = 4;

    REPEAT(x += y;
                   x *= y)
    REPEAT(x += y;
                   x *= y)

    for (int i = 0; i < ITERS; i++) {
        x *= y;
        x += y;
        x *= y;
        x *= y;
    }
    x *= y;
    x += y;
    x *= y;
    x *= y;
    //printf("You entered B\n");
    // printf("B\n");
}

void input_C() {
    int x = 3;
    int y = 4;
    REPEAT(x += y;
                   x *= y)
    REPEAT(x += y;
                   x *= y)

    for (int i = 0; i < ITERS; i++) {
        x *= y;
        x += y;
        x *= y;
        x *= y;
    }
    x *= y;
    x += y;
    x *= y;
    x *= y;
    //printf("You entered C\n");
    // printf("C\n");
}

void input_D() {
    int x = 3;
    int y = 4;
    REPEAT(x += y;
                   x *= y)
    REPEAT(x += y;
                   x *= y)

    for (int i = 0; i < ITERS; i++) {
        x *= y;
        x += y;
        x *= y;
        x *= y;
    }
    x *= y;
    x += y;
    x *= y;
    x *= y;
    //printf("You entered D\n");
    // printf("D\n");
}

int main(int argc, char **argv) {
    /*
        aim is to repeatedly execute a code segment (like a loop) 
        on a matching character input, increasing the probability
        of spy program to detect the input while the loop runs 
    */
    int i;
   //scanf("%d",&i);
   i = 0;
   if(!i){
    char rand_str[] = "ACDADCBCABDADDABAACCABDADCACCDDCBCDBADCCCACCABABACBCCDCCCBAADCDDCAABACAACDBACDACACCCBDBBAADCABACADCDABBCBCDDBADDBAADDCCABDACBAADBCCBBABDDCACBDAABBABDDADCBCDDABDCAABDADBDBBAADCACDBACCDBCDDBAADCAACCDADCAABDCAABCAADBCCBDBBDBDACDBBCACCDDDBADDBBAACBBADCDCABBBBCBCCBBBCADCBDABBBCBAADDCBADBACBCCABDCBAADDCCCCBBCBBABCBDACABDAACABADABBCDCADCCCDCCBCAADACCCBCDDCABCADDBABCCCCBDCBBBDBDAABCACCDACBAAADDCCABDCADDDBACBDACDCBABABDABCBBBDADDBCABAADDABACABBBABCDDCBBACCCACADCAABCCAAACAADBCCBCBDCDBDAACADCACDDDBCBBCCCDBDDBABADBCBCACBCCBABCACCBACABAADDAABDACACCADDCDBBABACADCCCBCADACDCCBCCCABBADDADCDBBDABCABCBDBBCAACBDCABABCCCDACCCDCABDDDADAAABBCCCBDADCCCACCADCDABCCBDBCCAAACDBAACCCBDAAAADCCAAAABABDBAADCDDBBAACCCDDCAADBADBCCCCDBBABACAADDDDBDADACCBADACCADCDDDACBCDCDDDCCACCBDAAAAABAABCDCDABCDBCCBDBCDACADACDDCDBBCDBCCDACBDCBACBDDBCCDDDCCBDBBCADDBCDDCACCCCADDDCBABDABCDDCCDCDCDDBBCCDCDDDDBBAACCDBABADCBBCCDCDACABABBBCADCDBDDBCADCCBDCBBAAADBCADDABBDCAADBDBCBCDCACDCBDABBDBBDBBDDBCABCCDBBBDACBDCAABBABCBABDAABCABBABDBBBACABDBBACBCAACDDDBCCDBACBBB";
    for(i=0;i<1024;i++){
        if (rand_str[i] != 'A' && rand_str[i] != 'B' && rand_str[i] != 'C' && rand_str[i] != 'D' && rand_str[i] != 'Q') {
            printf("You're doing it wrong\n");
        } else {

    #ifdef BYPASS_BRANCH_PREDICTION
                /* __builtin_expect tells the compiler that the mentioned possbilities
                    under BYPASS_BRANCH_PREDICTION are much more likely than the rest
                */
                if (__builtin_expect(rand_str[i] == 'A', 0)) {
                    input_A();
                } else if (__builtin_expect(rand_str[i] == 'B', 0)) {
                    input_B();
                } else if (__builtin_expect(rand_str[i] == 'C', 0)) {
                    input_C();
                } else if (__builtin_expect(rand_str[i] == 'D', 0)) {
                    input_D();
                } else {
                    exit(0);
                }
    #else
                if (rand_str[i] == 'A') {
                    input_A();
                } else if (rand_str[i] == 'B') {
                    input_B();
                } else if (rand_str[i] == 'C') {
                    input_C();
                } else if (rand_str[i] == 'D') {
                    input_D();
                } else {
                    exit(0);
                }
    #endif
            }
            long x, y;
            x = 5;
            y = 6;
            for (long i = 0; i < 160000000; ++i){
                x *= y;
                y *=x;
                if(i%7==0){
                    x = y/5;
                    y = (x*9)/11;
                }
                else if(i%20==0){
                    x = y%10;
                    y = x%20;
                }
            }
        }
   }
    else if(i){
        char input[16];

        while (1) {

            printf("Enter either A, B, C, D or Q to quit:\n");
            fgets(input, 16, stdin);

            if (input[0] != 'A' && input[0] != 'B' && input[0] != 'C' && input[0] != 'D' && input[0] != 'Q') {
                printf("You're doing it wrong\n");
            } else {

    #ifdef BYPASS_BRANCH_PREDICTION
                /* __builtin_expect tells the compiler that the mentioned possbilities
                    under BYPASS_BRANCH_PREDICTION are much more likely than the rest
                */
                if (__builtin_expect(input[0] == 'A', 0)) {
                    input_A();
                } else if (__builtin_expect(input[0] == 'B', 0)) {
                    input_B();
                } else if (__builtin_expect(input[0] == 'C', 0)) {
                    input_C();
                } else if (__builtin_expect(input[0] == 'D', 0)) {
                    input_D();
                } else {
                    exit(0);
                }
    #else
                if (input[0] == 'A') {
                    input_A();
                } else if (input[0] == 'B') {
                    input_B();
                } else if (input[0] == 'C') {
                    input_C();
                } else if (input[0] == 'D') {
                    input_D();
                } else {
                    exit(0);
                }
    #endif
            }

        }
    }
    printf("done\n");
}

