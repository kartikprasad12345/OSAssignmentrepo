#include <iostream>
#include <iterator>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h> 
void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads);
void parallel_for(int low1, int high1, int low2, int high2,
std::function<void(int, int)> &&lambda, int numThreads);
typedef struct {
  int hi;
  int lo;
  std::function<void(int)> lambda;
}Vector_argument;
void * function_for_thread_vector(void* arguments){
  Vector_argument * temp=(Vector_argument*) arguments;
  int high=temp->hi;
  int low=temp->lo;
  auto templam=temp->lambda;
  for(int i=low;i<high;i++){
    templam(i);
  }
  return  NULL;

}
void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads){
  pthread_t pid_of_threads[numThreads];
  Vector_argument all_args_vector[numThreads];
  int chunk_of_array=(high-low)/numThreads;
  for(int i=0;i<numThreads;i++){
    if(i==numThreads-1){
      all_args_vector[i].hi=high;
      all_args_vector[i].lo=(i)*chunk_of_array;
      all_args_vector[i].lambda=lambda;

    }else{
      all_args_vector[i].hi=(i+1)*chunk_of_array;
      all_args_vector[i].lo=(i)*(chunk_of_array);
      all_args_vector[i].lambda=lambda;
      // pthread_create(&pid_of_threads[i-1],NULL, function_for_thread_vector,(void*) temp); 
    }
    // temp->hi=i*chunk_of_array;
    // temp->lo=(i-1)*(chunk_of_array);
    // temp->lambda=lambda;
    pthread_create(&pid_of_threads[i],NULL, function_for_thread_vector,(void *)&all_args_vector[i]); 
  }
  for(int j=0;j<numThreads;j++){
    pthread_join(pid_of_threads[j], NULL);
  }
  return;
}





int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

int main(int argc, char **argv) {
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main


