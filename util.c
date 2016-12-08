#include "util.h"

//Linked list functions
int ll_get_length(LLnode * head)
{
    LLnode * tmp;
    int count = 1;
    if (head == NULL)
        return 0;
    else
    {
        tmp = head->next;
        while (tmp != head)
        {
            count++;
            tmp = tmp->next;
        }
        return count;
    }
}

void ll_append_node(LLnode ** head_ptr,
    void * value)
{
    LLnode * prev_last_node;
    LLnode * new_node;
    LLnode * head;
    
    if (head_ptr == NULL)
    {
        return;
    }
    
    //Init the value pntr
    head = (*head_ptr);
    new_node = (LLnode *) malloc(sizeof(LLnode));
    new_node->value = value;
    
    //The list is empty, no node is currently present
    if (head == NULL)
    {
        (*head_ptr) = new_node;
        new_node->prev = new_node;
        new_node->next = new_node;
    }
    else
    {
        //Node exists by itself
        prev_last_node = head->prev;
        head->prev = new_node;
        prev_last_node->next = new_node;
        new_node->next = head;
        new_node->prev = prev_last_node;
    }
}


LLnode * ll_pop_node(LLnode ** head_ptr)
{
    LLnode * last_node;
    LLnode * new_head;
    LLnode * prev_head;
    
    prev_head = (*head_ptr);
    if (prev_head == NULL)
    {
        return NULL;
    }
    last_node = prev_head->prev;
    new_head = prev_head->next;
    
    //We are about to set the head ptr to nothing because there is only one thing in list
    if (last_node == prev_head)
    {
        (*head_ptr) = NULL;
        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
    else
    {
        (*head_ptr) = new_head;
        last_node->next = new_head;
        new_head->prev = last_node;
        
        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
}

void ll_destroy_node(LLnode * node)
{
    if (node->type == llt_string)
    {
        free((char *) node->value);
    }
    free(node);
}

//Compute the difference in usec for two timeval objects
long timeval_usecdiff(struct timeval *start_time,
  struct timeval *finish_time)
{
    long usec;
    usec=(finish_time->tv_sec - start_time->tv_sec)*1000000;
    usec+=(finish_time->tv_usec- start_time->tv_usec);
    return usec;
}


//Print out messages entered by the user
void print_cmd(Cmd * cmd)
{
    fprintf(stderr, "src=%d, dst=%d, message=%s\n",
        cmd->src_id,
        cmd->dst_id,
        cmd->message);
}


char * convert_frame_to_char(Frame * frame)
{
    //TODO: You should implement this as necessary
    char * char_buffer = (char *) malloc(MAX_FRAME_SIZE);
    memset(char_buffer,
     0,
     sizeof(Frame));
    memcpy(char_buffer, frame, sizeof(Frame));
    // memcpy(char_buffer, &frame->sender, sizeof(frame->sender));
    // memcpy(char_buffer+2, &frame->receiver, sizeof(frame->receiver));
    // memcpy(char_buffer+4, &frame->seq, sizeof(frame->seq));
    // memcpy(char_buffer+5, &frame->ack, sizeof(frame->ack));
    // memcpy(char_buffer+6, frame->data, FRAME_PAYLOAD_SIZE);
    /*
     memcpy(char_buffer,
     frame->data,
     FRAME_PAYLOAD_SIZE);
     */
    return char_buffer;
}


Frame * convert_char_to_frame(char * char_buf)
{
    //TODO: You should implement this as necessary
    Frame * frame = (Frame *) malloc(sizeof(Frame));
    memset(frame, 0, sizeof(Frame));
    memcpy(frame, char_buf, sizeof(Frame));
    /*
     memset(frame->data,
     0,
     sizeof(char)*sizeof(frame->data));
     memcpy(frame->data,
     char_buf,
     sizeof(char)*sizeof(frame->data));
     */
    return frame;
}

void calculate_timeout(struct timeval * timeout){
    gettimeofday(timeout, NULL);
    timeout->tv_usec+=100000;
    if(timeout->tv_usec>=1000000){
        timeout->tv_usec-=1000000;
        timeout->tv_sec+=1;
    }
}


void ll_split_head(LLnode ** head_ptr, int payload_size){
    if(head_ptr == NULL || *head_ptr == NULL){
        return;
    }
    //get message from the head of the linked list
    LLnode* head = *head_ptr;
    Cmd* head_cmd = (Cmd*) head->value;
    char* msg = head_cmd->message;
    //do not need to split
    if(strlen(msg) < payload_size){
        return;
    }
    int i;
    LLnode* curr;
    //LLnode* next;
    Cmd* next_cmd;
    curr = head;
    for(i = payload_size; i < strlen(msg); i+=payload_size){
        //TODO: malloc here
        int cut_size=payload_size;
        if((strlen(msg) - i - payload_size) <= 0){
            cut_size = strlen(msg) - i;
        }
        char* cmd_msg = (char*) malloc((cut_size + 1) * sizeof(char));
        memset(cmd_msg, 0, (payload_size + 1) * sizeof(char));
        strncpy(cmd_msg, msg + i, payload_size);
        //TODO: fill the next_cmd
        //TODO: fill the next node and add it to the linked list
        //next = malloc(sizeof(LLnode));
        next_cmd = malloc(sizeof(Cmd));
        next_cmd->message=cmd_msg;
        next_cmd->src_id=head_cmd->src_id;
        next_cmd->dst_id=head_cmd->dst_id;
        //next->value=next_cmd;
        ll_append_node(&curr->next, next_cmd);
        curr=curr->next;
        //free(next_cmd);
    }
    msg[payload_size] = '\0';//cut the original msg
}

void print_frame(Frame* frame)
{
    fprintf(stderr, "\nframe:\n");
    fprintf(stderr, "frame->src=%d\n", frame->sender);
    fprintf(stderr, "frame->dst=%d\n", frame->receiver);
    fprintf(stderr, "frame->seq=%d\n", frame->seq);
    fprintf(stderr, "frame->ack=%d\n", frame->ack);
    fprintf(stderr, "frame->data=%s\n", frame->data);
    fprintf(stderr, "#frame--------\n");
}

void print_buffer(char* outgoing_charbuf){
    int i=0;
    for(;i<64;i++){
     fprintf(stderr, "outgoing buffer: %c\n", *(outgoing_charbuf+i));
 } 
}

char getCrc(char* array, int array_len){
  char crc = 0xFF;
  int i,j;
  for(i = 0; i<array_len; i++){
    crc ^= (char)array[i];
    for (j = 0; j < 8; j++)
    {
        if (crc & 0x01){
            crc = (crc>>1)^0x9c;
        }
        else{
            crc = (crc>>1);
        }
    }
}
return crc;
}
