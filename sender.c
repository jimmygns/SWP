#include "sender.h"

void init_sender(Sender * sender, int id)
{
    //TODO: You should fill in this function as necessary
    sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
    sender->LAR = -1;
    sender->LFS = -1;
}

void print_sender(Sender * sender)
{
    fprintf(stderr, "sender:\n");
    fprintf(stderr, "sender->id:%d\n",sender->send_id);
    fprintf(stderr, "sender->LAR=%d\n", sender->LAR);
    fprintf(stderr, "sender->LFS=%d\n", sender->LFS);
}

struct timeval * sender_get_next_expiring_timeval(Sender * sender)
{
    //TODO: You should fill in this function so that it returns the next timeout that should occur
    struct timeval* next_timeout;
    int j=0;
    int i=0;
    for(; i<8; i++){
        if(sender->sendQ[i].empty == 1){
            next_timeout = &(sender->sendQ[i].timeout);
            j=i+1;
            break;
        }
    }
    for(;j<8; j++){
        if(sender->sendQ[i].empty==1){
            long difference = timeval_usecdiff(next_timeout, &(sender->sendQ[j].timeout));
            if(difference<0){
                next_timeout = &(sender->sendQ[j].timeout);
            }
        }
    }
    return next_timeout;
}


void handle_incoming_acks(Sender * sender,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this sender
    //    5) Do sliding window protocol for sender/receiver pair
    int incoming_acks_length = ll_get_length(sender->input_framelist_head);
    while (incoming_acks_length>0) {
        LLnode* curr = ll_pop_node(&sender->input_framelist_head);
        incoming_acks_length = ll_get_length(sender->input_framelist_head);
        char* buffer = curr->value;
        Frame* frame = convert_char_to_frame(buffer);
        //int is_corr = is_corrupted(buffer, sizeof(buffer));
        char crc = getCrc(buffer, 64);
        curr = curr->next;
        free(buffer);
        //fprintf(stderr, "%s\n", "buffer freed");
        if(crc!=0){
            fprintf(stderr, "crc: %d\n", crc);
            fprintf(stderr, "%s\n", "corruption");
            free(frame);
            free(curr);
            continue;
        }

        if(frame->receiver != sender->send_id){
            fprintf(stderr, "%s\n", "id not equal");
            free(frame);
            free(curr);
            continue;
        }
        print_sender(sender);
        print_frame(frame);
        //print_frame(frame);
        unsigned char seq_num = frame->ack;
        unsigned char LAR = sender->LAR;
        unsigned char LFS = sender->LFS;
        
        int loop_size;
        if(LAR<=LFS&&seq_num>LAR&&seq_num<=LFS){
            loop_size = LFS - LAR;
        }
        else if(LAR>LFS&&(seq_num>LAR||seq_num<=LFS)){
            loop_size = 256 - LAR + LFS;
        }
        else{
            free(frame);
            free(curr);
            continue;
        }
        
        int position = seq_num % 8;
        if(position == (LAR+1) % 8){
            memset(&sender->sendQ[position], 0, sizeof(sendQ_slot));
            LAR++;
            sender->LAR = LAR;
        }
        else{
            int i = 0;
            for(;i<loop_size;i++){
                int LAR_position=(LAR+1)%8;
                if(LAR_position==position){
                    memset(&sender->sendQ[LAR_position], 0, sizeof(sendQ_slot));
                    LAR++;
                    break;
                }
                if(sender->sendQ[LAR_position].empty==1){
                    memset(&sender->sendQ[LAR_position], 0, sizeof(sendQ_slot));
                    LAR++;
                }
            }
            sender->LAR = LAR;
        }
        
    }
}


void handle_input_cmds(Sender * sender,
                       LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame

    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    
        
    //Recheck the command queue length to see if stdin_thread dumped a command on us
    input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    while (input_cmd_length > 0)
    {
        //printf("input_cmd_length:%d\n", input_cmd_length);
        //Pop a node off and update the input_cmd_length
        int position = (sender->LFS+1) % 8;
        if(sender->sendQ[position].empty==1){
            fprintf(stderr, "%s\n", "sender queue full");
            print_sender(sender);
            int i = 0;
            for(; i<8; i++){
                if(sender->sendQ[i].empty!=1){
                    fprintf(stderr, "empty at %d\n", i);
                    continue;
                }
                print_frame(&sender->sendQ[i].frame);
            }
            break;
        }

        LLnode * ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
        input_cmd_length = ll_get_length(sender->input_cmdlist_head);

        //Cast to Cmd type and free up the memory for the node
        Cmd * outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
        

        print_cmd(outgoing_cmd);

        //DUMMY CODE: Add the raw char buf to the outgoing_frames list
        //NOTE: You should not blindly send this message out!
        //      Ask yourself: Is this message actually going to the right receiver (recall that default behavior of send is to broadcast to all receivers)?
        //                    Does the receiver have enough space in in it's input queue to handle this message?
        //                    Were the previous messages sent to this receiver ACTUALLY delivered to the receiver?
        int msg_length = strlen(outgoing_cmd->message);
        if (msg_length > FRAME_PAYLOAD_SIZE)
        {
            //Do something about messages that exceed the frame size
            //printf("<SEND_%d>: sending messages of length greater than %d is not implemented\n", sender->send_id, MAX_FRAME_SIZE);
            ll_append_node(&sender->input_cmdlist_head,outgoing_cmd);
            ll_split_head(&sender->input_cmdlist_head,FRAME_PAYLOAD_SIZE-1);
        }
        else
        {
            //This is probably ONLY one step you want
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            strcpy(outgoing_frame->data, outgoing_cmd->message);
            outgoing_frame->sender = sender->send_id;
            outgoing_frame->receiver=outgoing_cmd->dst_id;
            sender->LFS++;
            outgoing_frame->seq=sender->LFS;
            outgoing_frame->ack=sender->LFS;
            
            //At this point, we don't need the outgoing_cmd
            free(outgoing_cmd->message);
            free(outgoing_cmd);

            //Convert the message to the outgoing_charbuf
            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            print_frame(outgoing_frame);
            char crc = getCrc(outgoing_charbuf, 63);
            outgoing_frame->crc_footer = crc;
            print_frame(outgoing_frame);
            memset(&sender->sendQ[position], 0, sizeof(sendQ_slot));
            sender->sendQ[position].empty = 1;
            memcpy(&sender->sendQ[position].frame, outgoing_frame, sizeof(Frame));
            calculate_timeout(&sender->sendQ[position].timeout);
            outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            //append_crc(outgoing_charbuf, MAX_FRAME_SIZE);
            ll_append_node(outgoing_frames_head_ptr,
                           outgoing_charbuf);
            print_sender(sender);
            //print_frame(outgoing_frame);
            //print_buffer(outgoing_charbuf);
            free(outgoing_frame);
        }
        free(ll_input_cmd_node);
    }   
}


void handle_timedout_frames(Sender * sender,
                            LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames
    struct timeval frame_timeout;
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    //Frame* frame;
    int i = 0;
    for(; i<8; i++){
        if(sender->sendQ[i].empty!=1){
            continue;
        }
        frame_timeout = sender->sendQ[i].timeout;
        long difference = timeval_usecdiff(&current_time, &frame_timeout);
        if(difference <= 0){
            calculate_timeout(&sender->sendQ[i].timeout);
            //frame = &sender->sendQ[i].frame;
            char* buf = convert_frame_to_char(&(sender->sendQ[i].frame));
            print_sender(sender);
            print_frame(&(sender->sendQ[i].frame));
            ll_append_node(outgoing_frames_head_ptr, buf);
        }
    }
}


void * run_sender(void * input_sender)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender * sender = (Sender *) input_sender;    
    LLnode * outgoing_frames_head;
    struct timeval * expiring_timeval;
    long sleep_usec_time, sleep_sec_time;
    
    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages

    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);

    while(1)
    {    
        outgoing_frames_head = NULL;

        //Get the current time
        gettimeofday(&curr_timeval, 
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        if (expiring_timeval == NULL)
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time/1000000;
                sleep_usec_time = sleep_usec_time % 1000000;   
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time*1000;
            }   
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        
        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);
        
        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0)
        {
            
            pthread_cond_timedwait(&sender->buffer_cv, 
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        //Implement this
        handle_incoming_acks(sender,
                             &outgoing_frames_head);

        //Implement this
        handle_input_cmds(sender,
                          &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);


        //Implement this
        handle_timedout_frames(sender,
                               &outgoing_frames_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *)  ll_outframe_node->value;

            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
