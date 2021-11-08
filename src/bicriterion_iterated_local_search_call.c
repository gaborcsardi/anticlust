#include <stdlib.h> 
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "header.h"
#include <R.h>

double uniform_rnd_number();
double uni_rnd_number_range(double min, double max);

// generate a random uniform number in range
double uni_rnd_number_range(double min, double max) {
  double number = uniform_rnd_number();
  return (min + number * (max - min));
}

double uniform_rnd_number() {
  GetRNGstate();
  double my_number = unif_rand();
  PutRNGstate();
  return(my_number);
}

//datastructure to store a linked-list of partitions
struct Pareto_element { 
  double diversity; 
  double dispersion;
  int *partition;
  struct Pareto_element *next;
};


//receive data from r, call bils algorithm, save results for r
void bicriterion_iterated_local_search_call(double *distances, int *N, int *R, 
                                            int *upper_bound, int *WL, double *W, double *Xi, 
                                            int *partition,
                                            double *result
                                           ) {
  
  const int n = *N; // number of elements
  const int r = *R; // number of restarts
  const int u = *upper_bound; //max. length of result-list
  const int wl = *WL; // length of possible weights
  
  int distance_ptr[n]; // indexing columns
  double distance_pts[n][n];
  
  // Column offsets (to convert one-dimensional array to Row/Col major)
  for(int i = 0; i < n; i++) {
     distance_ptr[i] = i * n;
  }
  
  // Reconstruct the data points as N x N matrix
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < n; j++) {
      distance_pts[i][j] = distances[distance_ptr[j]++];
    }
  }
  
  double weights[wl];
  for(int i = 0; i < wl; i++){
    weights[i] = W[i];
  }
  
  double neighbor_percent[2];
  neighbor_percent[0] = Xi[0];
  neighbor_percent[1] = Xi[1];
  
  //divide restarts for both parts of the algorithm equally
  int half_restarts = r/2 + (r%2);
  
  struct Pareto_element* head = multistart_bicriterion_pairwise_interchange(n, distance_pts, half_restarts, wl, weights, partition);
  head = bicriterion_iterated_local_search(head, n, distance_pts, half_restarts, wl, weights, neighbor_percent);
  
  //fill result with -1 to identify the last partition in the matrix
  for(int i = 0; i < n*u; i++){
    result[i] = -1;
  }
  
  //update result with the solutions from the linked list
  int position = 0;
  while (head != NULL && position != n*u) {
    int *array = head->partition;
    for(int i = 0; i < n; i++){
      result[position] = array[i];
      position++;
    }
    head = head->next;
  } 
}
  

struct Pareto_element* multistart_bicriterion_pairwise_interchange(size_t N, double matrix[N][N], int R, int WL, 
                                                                   double weights[WL], int *partition) {
  
  struct Pareto_element* head = NULL; // head pointing on the later linked list(paretoset)
  
  for(int a = 0; a < R; a++){
    if (a > 0) {
        shuffle_permutation(N, partition);
    }
    double div_weight = sample(WL, weights); 
    double dis_weight = 1 - div_weight;
    double diversity = get_diversity(N, partition, matrix);
    double save_diversity = diversity;
    double dispersion = get_dispersion(N, partition, matrix);
    double save_dispersion = dispersion;
    double max_bicriterion = div_weight*diversity + dis_weight*dispersion;
    bool Flag = false;
    while(!Flag){
      Flag = true;
      for(int i = 0; i < N-1; i++){
        for(int j = i+1; j < N; j++){
          int g = partition[i];
          int h = partition[j];
          if(g != h){
            cluster_swap(N, i, j, partition);
            double current_diversity = get_diversity_fast(save_diversity, i, j, N, partition, matrix);
            double current_dispersion = get_dispersion_fast(save_dispersion , i, j, N, partition, matrix);
            update_pareto(&head, N, partition,current_diversity, current_dispersion);
            double current_bicriterion = div_weight*current_diversity + dis_weight*current_dispersion;
            if(current_bicriterion > max_bicriterion){
              save_diversity = current_diversity;
              save_dispersion = current_dispersion;
              max_bicriterion = current_bicriterion;
              Flag = false;
            }else{
              cluster_swap(N, i, j, partition);
            }
          }
        }
      }
    }
  }
  return (head);
}  


struct Pareto_element* bicriterion_iterated_local_search(struct Pareto_element* head, size_t N, double matrix[N][N], int R, int WL, double weights[WL], double neighbor_percent[2]){
  
  for(int a = 0; a < R; a++){
    double div_weight = sample(10, weights); 
    double dis_weight = 1 - div_weight;
    double neighborhood_size = uni_rnd_number_range(neighbor_percent[0], neighbor_percent[1]);
    int partition[N];
    linked_list_sample(head, N, partition);
    for(int i = 0; i < N-1; i++){
      for(int j = i + 1; j < N; j++){
        int g = partition[i];
        int h = partition[j];
        if(g != h){
          double random = uni_rnd_number_range(0,1);
          if(random < neighborhood_size){
            cluster_swap(N, i, j, partition);
          }
        }
      }
    }
    double diversity = get_diversity(N, partition, matrix);
    double save_diversity = diversity;
    double dispersion = get_dispersion(N, partition, matrix);
    double save_dispersion = dispersion;
    double max_bicriterion = div_weight*diversity + dis_weight*dispersion;
    bool Flag = false;
    while(!Flag){
      Flag = true;
      for(int i = 0; i < N-1; i++){
        for(int j = i+1; j < N; j++){
          int g = partition[i];
          int h = partition[j];
          if(g != h){
            cluster_swap(N, i, j, partition);
            double current_diversity = get_diversity_fast(save_diversity, i, j, N, partition, matrix);
            double current_dispersion = get_dispersion_fast(save_dispersion , i, j, N, partition, matrix);
            update_pareto(&head, N, partition, current_diversity, current_dispersion);
            double current_bicriterion = div_weight*current_diversity + dis_weight*current_dispersion;
            if(current_bicriterion > max_bicriterion){
              save_diversity = current_diversity;
              save_dispersion = current_dispersion;
              max_bicriterion = current_bicriterion;
              Flag = false;
            }else{
              cluster_swap(N, i, j, partition);
            }
          }
        }
      }
    }
  }
  return (head);
}     

//sample functin from R. Get one random element from an array
double sample(size_t array_size, double array[array_size]) {
  int r = rand() % array_size;
  return(array[r]);
}

double get_diversity(size_t N, int partition[N], double matrix[N][N]){
  
  double sum = 0;
  
  for(size_t i = 0; i < N-1; i++){
    for(size_t j = i+1; j < N; j++){
      if(partition[i] == partition[j]){
        sum = sum + matrix[i][j];
      }
    }
  }
  
  return(sum);  
}


double get_dispersion(size_t N, int partition[N], double matrix[N][N]){
  
  double min = INFINITY;
  double distance;
  
  for(size_t i = 0; i < N-1; i++){
    for(size_t j = i+1; j < N; j++){
      if(partition[i] == partition[j]){
        distance = matrix[i][j];
        if(distance < min){
          min = distance;
        }
      }
    }
  }
  
  return(min);
}


// Fisher-Yates shuffle algorithm for shuffling ermutations
void shuffle_permutation(int N, int *permutation) {
    for (int i = 0; i <= N-2; i++) {
        int j = i + (rand() % (N-i));
        cluster_swap(N, i, j, permutation);
    }
}

void cluster_swap(size_t N, int i, int j, int partition[N]){
  int save = partition[i];
  partition[i] = partition[j];
  partition[j] = save;
}


void update_pareto(struct Pareto_element** head_ref, size_t N, int partition[N], double diversity, double dispersion){
  
  if(*head_ref == NULL){
    compress(N, partition);
    push(head_ref, diversity, dispersion, N, partition);
    
  }else{
    if(!(paretodominated(*head_ref, diversity, dispersion))){
      compress(N, partition);
      if(!(paretoincluded(*head_ref, N, partition))){
        delete_outdated(head_ref, diversity, dispersion);
        push(head_ref, diversity, dispersion, N, partition);
      }
    }
  }
}


//compress partition to uniform ascending order
void compress(size_t N, int partition[N]){
  
  int cluster = 0;
  int change;
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster){
      cluster++;
    }else if(partition[i] > cluster){
      change = partition[i];
      for(int j = i; j < N; j++){
        if(change == partition[j]){
          partition[j] = cluster;
        }else if(cluster == partition[j]){
          partition[j] = change;          
        }
      }
      cluster++;
    }
  } 
}


bool paretodominated(struct Pareto_element* head, double diversity, double dispersion){
  
  while(head != NULL){
    if((head->diversity >= diversity && head->dispersion > dispersion) || (head->diversity > diversity && head->dispersion >= dispersion)){
      return (true);
    }
    head = head->next;
  }
  
  return (false);
}


bool paretoincluded(struct Pareto_element* head, size_t N, int partition[N]){
  
  while(head != NULL){
    int * array = head->partition;
    int counter = 0;
    for(int i = 0; i < N; i++){
      if(partition[i] == array[i]){
        counter++;
      }
    }
    if(N == counter){
      return(true);
    }
    head = head->next;
  }
  
  return(false);
}


void push(struct Pareto_element** head_ref, double diversity, double dispersion, size_t N, int partition[N]) { 
  
        /* 1. allocate node */
        struct Pareto_element* new_node = (struct Pareto_element*) malloc(sizeof(struct Pareto_element)); 
        
        /* 2. put in the data  */
        new_node->diversity  = diversity;
        new_node->dispersion = dispersion;
        new_node->partition =(int *)malloc(sizeof(int)*N ) ;  // vllt memcpy anstatt schleife?
        for(int b = 0;b<N;b++){ 
            new_node->partition[b]  = partition[b];
        }
        
        /* 3. Make next of new node as head */
        new_node->next = (*head_ref); 
            
        /* 4. move the head to point to the new node */
        (*head_ref)    = new_node; 
}  


void delete_outdated(struct Pareto_element** head_ref, double diversity, double dispersion){
  
        struct Pareto_element* temp = *head_ref;
        struct Pareto_element* prev = *head_ref;
        struct Pareto_element* safe = NULL;
        
        while(temp != NULL && ((diversity >= temp->diversity && dispersion > temp->dispersion) || 
            (diversity > temp->diversity && dispersion >= temp->dispersion))){
            *head_ref = temp->next; 
            prev = temp->next;
            free(temp); 
            temp = prev;
        }
        if(temp != NULL){
            temp = temp->next;
        }
        
        while(temp != NULL){
            if((diversity >= temp->diversity && dispersion > temp->dispersion) || 
                (diversity > temp->diversity && dispersion >= temp->dispersion)){
            prev->next = temp->next;
            safe = temp->next;  
            free(temp);
            temp = safe;
            }else{
            prev = prev->next;
            temp = temp->next;
            }
        }
} 


// selects a random partition
void linked_list_sample(struct Pareto_element* head, size_t N, int* partition){
  
  int count = linked_list_length(head);
  int r = rand() % count;
  struct Pareto_element* current = head; 
  
  for(int i = 0; i < r; i++){
    current = current->next;
  }
  
  for(int i = 0; i < N; i++){
    partition[i] = current->partition[i];
  }
  
}

int linked_list_length(struct Pareto_element* head) {
  
  int count = 0;   
  struct Pareto_element* current = head;  
  while (current != NULL) 
  { 
    count++; 
    current = current->next; 
  } 
  return count; 
} 


double get_diversity_fast(double diversity, int x, int y, size_t N, int partition[N], double matrix[N][N]){
  
  int cluster_x = partition[x];
  int cluster_y = partition[y];
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster_y && i != x && i != y){
      diversity -= matrix[i][x];
    }
  }
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster_x && i != x){
      diversity += matrix[i][x];
    }
  }
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster_x && i != x && i != y){
      diversity -= matrix[i][y];
    }
  }
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster_y && i != y){
      diversity += matrix[i][y];
    }
  }
  
  return(diversity);  
}


double get_dispersion_fast(double dispersion, int x,int y, size_t N, int partition[N], double matrix[N][N]){
  
  int cluster_x = partition[x];
  int cluster_y = partition[y];
  bool before = false;
  bool after = false;
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster_y  && i != x && i != y){
      if(dispersion == matrix[i][x]){
        before = true;
        break;
      }
    }
  }
  
  for(int i = 0; i < N && (!before); i++){
    if(partition[i] == cluster_x && i != x && i != y){
      if(dispersion == matrix[i][y]){
        before = true;
        break;
      }
    }
  }
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster_x && i != x){
      if(dispersion >= matrix[i][x]){
        dispersion = matrix[i][x];
        after = true;
      }
    }
  }
  
  for(int i = 0; i < N; i++){
    if(partition[i] == cluster_y && i != y){
      if(dispersion >= matrix[i][y]){
        dispersion = matrix[i][y];
        after = true;
      }
    }
  }
  
  
  if(!before && !after){
    return(dispersion);
  }
  
  if(!before && after){
    return(dispersion);
  }
  
  if(before && after){
    return(dispersion);
  }
  
  if(before && !after){
    dispersion = get_dispersion(N, partition, matrix);
    return(dispersion);
  }
  
  return(dispersion);
}
