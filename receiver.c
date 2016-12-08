#include "receiver.h"


void print_receiver(Receiver * receiver)
{
    fprintf(stderr, "receiver:\n");
    fprintf(stderr, "receiver->id:%d\n",receiver->recv_id);
    fprintf(stderr, "receiver->NFE=%d\n", receiver->NFE);
    fprintf(stderr, "receiver->LAF=%d\n", receiver->LAF);
}

void init_receiver(Receiver * receiver,
                   int id)
{
    receiver->recv_id = id;
    receiver->input_framelist_head = NULL;
    receiver->NFE = 0;
    receiver->LFR = -1;
    receiver->LAF = 7;
}

Frame * build_frame(Receiver* receiver, Frame* inframe){
    Frame* outframe = (Frame*) malloc(sizeof(Frame));
    memset(outframe, 0, sizeof(Frame));
    outframe->sender = inframe->receiver;
    outframe->receiver = inframe->sender;
    outframe->ack = inframe->seq;
    return outframe;
}

void handle_incoming_msgs(Receiver * receiver,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this receiver
    //    5) Do sliding window protocol for sender/receiver pair

    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    while (incoming_msgs_length > 0)
    {
        
        //Pop a node off the front of the link list and update the count
        LLnode * ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        //fprintf(stderr, "incoming_msgs_length: %d\n", incoming_msgs_length);
        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        
        
        
        //printf("msg length:%d\n", incoming_msgs_length);
        //printf("<RECV_%d>:[%s]\n", receiver->recv_id, inframe->data);
        char crc = getCrc(raw_char_buf, 64);
        
        if(crc!=0){
        //if(is_corrupted(raw_char_buf, MAX_FRAME_SIZE)==1){
            free(inframe);
            free(ll_inmsg_node);
            fprintf(stderr, "crc: %d\n", crc);
            fprintf(stderr, "corrupted\n");
            continue;
        }

        if(inframe->receiver != receiver->recv_id){
            free(inframe);
            free(ll_inmsg_node);
            fprintf(stderr, "id not equal\n");
            continue;
        }
        
        
        
        
        
        
        print_receiver(receiver);
        //Free raw_char_buf
        //print_buffer(raw_char_buf);
        free(raw_char_buf);
        //print the incoming frame
        print_frame(inframe);
        unsigned char seq_num = inframe->seq;
        unsigned char NFE = receiver->NFE;
        unsigned char LAF = receiver->LAF;
        
        int loop_size;
        if(LAF>=NFE && seq_num>=NFE&&seq_num<=LAF){
            loop_size = LAF-NFE;
        }
        else if(LAF<NFE&&(seq_num>=NFE||seq_num<=LAF)){
            loop_size = 257 - NFE + LAF;
        }
        else{
            Frame* outframe = malloc(sizeof(Frame));
            char* outbuf;
            memset(outframe, 0, sizeof(Frame));
            outframe->sender = receiver->recv_id;
            outframe->receiver = inframe->sender;
            outframe->ack = receiver->LFR;
            outbuf = convert_frame_to_char(outframe);
            char crc = getCrc(outbuf,63);
            outframe->crc_footer=crc;
            outbuf = convert_frame_to_char(outframe);
            //append_crc(outbuf, sizeof(Frame));
            print_frame(outframe);
            ll_append_node(outgoing_frames_head_ptr, outbuf);
            free(outframe);
            free(inframe);
            free(ll_inmsg_node);
            continue;
        }
        
        int position = seq_num % 8;
        int NFE_position = NFE % 8;
        if (position == NFE_position) {
            printf("<RECV_%d>:[%s]\n", receiver->recv_id, inframe->data);
            Frame* outframe;
            char* outbuf;
            //printf("loop size:%d\n", loop_size);
            NFE++;
            int i = 0;
            //fprintf(stderr, "%d\n", i);
            for(;i<=loop_size;i++){
                //fprintf(stderr, "%d\n", i);
                NFE_position = NFE % 8;
                if(receiver->recvQ[NFE_position].received != 1){ 
                    break;
                }
                if(receiver->recvQ[NFE_position].received==1){
                    printf("<RECV_%d>:[%s]\n", receiver->recv_id, receiver->recvQ[NFE_position].frame.data);
                    fprintf(stderr, "%s\n", "previously received messages:");
                    print_frame(&receiver->recvQ[NFE_position].frame);
                    memset(&receiver->recvQ[NFE_position], 0, sizeof(recvQ_slot));
                    NFE++;
                }
            }
            receiver->NFE=NFE;
            receiver->LAF=NFE+7;
            receiver->LFR=NFE-1;
            outframe = malloc(sizeof(Frame));
            memset(outframe, 0, sizeof(Frame));
            outframe->sender = receiver->recv_id;
            outframe->receiver = inframe->sender;
            outframe->ack = receiver->LFR;
            print_frame(outframe);
            outbuf = convert_frame_to_char(outframe);
            char crc = getCrc(outbuf, 63);
            outframe->crc_footer = crc;
            outbuf = convert_frame_to_char(outframe);
            //append_crc(outbuf, sizeof(Frame));
            ll_append_node(outgoing_frames_head_ptr, outbuf);
            //print_buffer(outbuf);
            free(outframe);
        }
        else{
            receiver->recvQ[position].received = 1;
            memcpy(&receiver->recvQ[position].frame, inframe, sizeof(Frame));
            fprintf(stderr, "%s\n", "put into the buffer:");
            print_frame(&receiver->recvQ[position].frame);
        }
        
        //printf("<RECV_%d>:[%s]\n", receiver->recv_id, inframe->data);
        free(inframe);
        free(ll_inmsg_node);
        //fprintf(stderr, "%s\n", "freed inframe and node");
    }
}

void * run_receiver(void * input_receiver)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver * receiver = (Receiver *) input_receiver;
    LLnode * outgoing_frames_head;


    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages

    pthread_cond_init(&receiver->buffer_cv, NULL);
    pthread_mutex_init(&receiver->buffer_mutex, NULL);

    while(1)
    {    
        //NOTE: Add outgoing messages to the outgoing_frames_head pointer
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval, 
                     NULL);

        //Either timeout or get woken up because you've received a datagram
        //NOTE: You don't really need to do anything here, but it might be useful for debugging purposes to have the receivers periodically wakeup and print info
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //Check whether anything arrived
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0)
        {
            //Nothing has arrived, do a timed wait on the condition variable (which releases the mutex). Again, you don't really need to do the timed wait.
            //A signal on the condition variable will wake up the thread and reacquire the lock
            pthread_cond_timedwait(&receiver->buffer_cv, 
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);

        pthread_mutex_unlock(&receiver->buffer_mutex);
        
        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames user has appended to the outgoing_frames list
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *) ll_outframe_node->value;
            
            //The following function frees the memory for the char_buf object
            send_msg_to_senders(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);

}
