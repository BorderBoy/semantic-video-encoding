#include "roi_detection.h"

static zsock_t* requester = NULL;
static int frame_counter = 0;
static float total_time = 0; 

static inline double curtime(){
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec * 1e-9;
}

int detect_roi(x264_t *h, x264_frame_t *frame, pixel* roi_map){
    if (requester == NULL){
        x264_log( h, X264_LOG_INFO, "Connecting to ROI detector...\n");
        requester = zsock_new (ZMQ_REQ);
        zsock_connect (requester, "tcp://localhost:5555");
    }

    // double start = curtime();
    // Send planes
    send_planes(frame, requester);

    // Receive response
    zframe_t* message = zframe_recv (requester);
    // printf ("Received: %d bytes\n", zframe_size (message));

    memcpy(roi_map, zframe_data(message), zframe_size(message)); // roi map should have 1/16 of the size of the frame

    zframe_destroy (&message);
    // zsock_destroy (&requester);

    // double end = curtime();
    // total_time += end - start;
    // frame_counter++;
    
    return 0;
}

int send_planes(x264_frame_t *frame, zsock_t* requester){
    // Send image
    zmsg_t *msg = zmsg_new();

    add_plane_to_message(msg, frame, 0);
    add_plane_to_message(msg, frame, 1);

    int error = zmsg_send(&msg, requester);

    if(error != 0){
        return -1;
    }

    return 0;
}

void add_plane_to_message(zmsg_t *msg, x264_frame_t *frame, int index){
    // printf ("Sending plane %d, height: %d, width: %d, stride: %d\n", index, frame->i_lines[index], frame->i_width[index], frame->i_stride[index]);
    zframe_t* image_lines = zframe_new(&frame->i_lines[index], sizeof(int));
    zframe_t* image_width = zframe_new(&frame->i_width[index], sizeof(int));
    zframe_t* image_stride = zframe_new(&frame->i_stride[index], sizeof(int));

    bool interleaved = index == 1;
    zframe_t* image_interleaved = zframe_new(&interleaved, sizeof(bool));

    zframe_t* image = zframe_new(frame->plane[index], frame->i_stride[index] * frame->i_lines[index]);

    zmsg_append(msg, &image_lines);
    zmsg_append(msg, &image_width);
    zmsg_append(msg, &image_stride);
    zmsg_append(msg, &image_interleaved);
    zmsg_append(msg, &image);
}

void destroy_roi_detection(){
    zsock_destroy (&requester);

    // printf("ROI detection time/frame: %f ms\n", total_time/frame_counter * 1000);
}