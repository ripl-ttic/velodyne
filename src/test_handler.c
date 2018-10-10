#include <stdio.h>
#include <inttypes.h>

#include <lcm/lcm.h>

#include <bot_core/bot_core.h>
#include <bot_param/param_client.h>
#include <velodyne/velodyne.h>
#include <path_utils/path_util.h>
#include <lcmtypes/bot_param_update_t.h>

#include "lcmtypes/velodyne_t.h"
#include "lcmtypes/velodyne_list_t.h"

#define GAMMA 0.1
#define PUBLISH_HZ 50

typedef struct _rate_t {
  double current_hz;
  int64_t last_tick;
  int64_t tick_count;
} rate_t;

enum {
    COLOR_Z,
    COLOR_INTENSITY,
    COLOR_NONE,
};


typedef struct _pose_data_t pose_data_t;
struct _pose_data_t {
    double pose[6];
    double motion[6];
    int64_t utime;
};

typedef struct _state_t {

    GMainLoop *mainloop;

    rate_t* capture_rate;

    lcm_t *lcm;
    BotParam *param;

    int64_t last_collector_utime;

    int have_data;

    velodyne_calib_t *calib;
    velodyne_laser_return_collector_t *collector;
    //velodyne_state_t *vstate;

    GAsyncQueue *velodyne_message_queue;
    GThread *velodyne_work_thread;
    GMutex *mutex;
    int velodyne_work_thread_exit;

    int64_t last_publish_utime;

    int64_t last_msg_count;

    int64_t 	      last_velodyne_data_utime;
    int64_t           last_pose_utime;

} state_t;



rate_t* rate_new()
{
  rate_t* rt = (rate_t *) calloc(1, sizeof(rate_t));
  return rt;
}

void rate_destroy(rate_t* rate)
{
  free(rate);
}

int rate_update(rate_t* rate)
{
  // check the current time
  int64_t c_utime = bot_timestamp_now();

  // compute the framerate if we were to publish an image
  int64_t dt = c_utime - rate->last_tick;

  double p_framerate = GAMMA * (1.0 * 1e6 / dt) + (1 - GAMMA) * rate->current_hz;

  // otherwise, update current_hz with a exponential moving average, and return 1
  rate->current_hz = p_framerate;
  rate->last_tick = c_utime;
  rate->tick_count++;
  return 1;
}


static void
on_velodyne(const lcm_recv_buf_t *rbuf, const char * channel,
	    const velodyne_t * msg, void * user)
{
    state_t *self = (state_t *)user;
    g_assert(self);

    g_async_queue_push (self->velodyne_message_queue, velodyne_t_copy (msg));

    return;

    int64_t now = bot_timestamp_now();

    //we are missing data packets
    //static int count = 0;
    //count++;


    fprintf (stdout, "---------------------------------------------------------\n");

    // Is this a scan packet?
    if (msg->packet_type == SENLCM_VELODYNE_T_TYPE_DATA_PACKET) {

        fprintf(stdout,"time : %f \n", msg->utime /1.0e6);

        /*int i_f = 0;

        double ctheta = VELODYNE_GET_ROT_POS(msg->data, VELODYNE_DATA_FIRING_START(i_f));

        static double ctheta_prev = 0;

        static double delta = 0;

        if(fabs(fabs(ctheta - ctheta_prev) - delta) > 0.1 && fabs(fabs(ctheta - ctheta_prev) - delta) < 4.0 ){
            fprintf(stderr,"%f Different Delta : %f, %f\n", msg->utime /1.0e6, fabs(ctheta - ctheta_prev), delta);
        }
        delta = fabs(ctheta - ctheta_prev);

        ctheta_prev = ctheta;
        */
        /*velodyne_laser_return_collection_t *lrc =
            velodyne_decode_data_packet(self->calib, msg->data, msg->datalen, msg->utime);

        int ret = velodyne_collector_push_laser_returns (self->collector, lrc);

        velodyne_free_laser_return_collection (lrc);

        if (VELODYNE_COLLECTION_READY == ret) {
            //fprintf(stderr, "Outer count : %d\n", count);
            //count = 0;

            rate_update(self->capture_rate);

            fprintf(stderr,"Data rate : %f\n", self->capture_rate->current_hz);

            velodyne_laser_return_collection_t *lrc =
                velodyne_collector_pull_collection (self->collector);

            // if enough time has elapsed since the last scan push it onto the circular buffer

                // memory leak city if this isnt here as soon as you increase the history spacing
            velodyne_free_laser_return_collection (lrc);
            }*/
    }
}


static void
on_velodyne_debug(const lcm_recv_buf_t *rbuf, const char * channel,
                  const velodyne_list_t * msgl, void * user)
{
    state_t *self = (state_t *)user;
    g_assert(self);

    velodyne_t *msg;

    int64_t now = bot_timestamp_now();

    for (int i=0; i < msgl->num_packets; i++)
        g_async_queue_push (self->velodyne_message_queue, velodyne_t_copy (& (msgl->packets[i])));

    //g_async_queue_push (self->velodyne_message_queue, velodyne_t_copy (msg));
    return;

    //we are missing data packets
    //static int count = 0;
    //count++;

    // Is this a scan packet?
    /* if (msg->packet_type == SENLCM_VELODYNE_T_TYPE_DATA_PACKET) { */

    /*     int i_f = 0;  */

    /*     double ctheta = VELODYNE_GET_ROT_POS(msg->data, VELODYNE_DATA_FIRING_START(i_f)); */

    /*     static double test_handler_ctheta_prev = 0;  */

    /*     static double delta = 0; */

    /*     if(fabs(fabs(ctheta - test_handler_ctheta_prev) - delta) > 0.1 && fabs(fabs(ctheta - test_handler_ctheta_prev) - delta) < 4.0 ){ */
    /*         //fprintf(stderr,"%f Different Delta : %f, %f\n", msg->utime /1.0e6, fabs(ctheta - test_handler_ctheta_prev), delta); */
    /*     } */
    /*     delta = fabs(ctheta - test_handler_ctheta_prev); */

    /*     test_handler_ctheta_prev = ctheta; */

    /*     // Decode the data packet */
    /* 	fprintf (stdout, "*******************************************************\n"); */
    /*     velodyne_laser_return_collection_t *lrc = */
    /*         velodyne_decode_data_packet(self->calib, msg->data, msg->datalen, msg->utime); */

    /*     int ret = -1; */
    /*     //int ret = velodyne_collector_push_laser_returns (self->collector, lrc); */

    /*     velodyne_free_laser_return_collection (lrc); */

    /*     return; */

    /*     if (VELODYNE_COLLECTION_READY == ret) { */
    /*         //fprintf(stderr, "Outer count : %d\n", count);  */
    /*         //count = 0; */

    /*         rate_update(self->capture_rate); */

    /*         fprintf(stderr,"Data rate : %f\n", self->capture_rate->current_hz); */

    /*         velodyne_laser_return_collection_t *lrc = */
    /*             velodyne_collector_pull_collection (self->collector); */

    /*         // if enough time has elapsed since the last scan push it onto the circular buffer */

    /*             // memory leak city if this isnt here as soon as you increase the history spacing */
    /*         velodyne_free_laser_return_collection (lrc); */
    /*     } */
    /*     else if(VELODYNE_COLLECTION_READY == ret) { */
    /*         rate_update(self->capture_rate); */

    /*         fprintf(stderr,"Data rate : %f\n", self->capture_rate->current_hz); */

    /*         velodyne_laser_return_collection_t *lrc = */
    /*             velodyne_collector_pull_collection (self->collector); */

    /*         // if enough time has elapsed since the last scan push it onto the circular buffer */

    /*             // memory leak city if this isnt here as soon as you increase the history spacing */
    /*         velodyne_free_laser_return_collection (lrc); */
    /*     } */
    /* } */
}

static void
process_velodyne (state_t *self, const velodyne_t *v)
{

    //we are missing data packets
    //static int count = 0;
    //count++;

    // Is this a scan packet?
    if (v->packet_type == SENLCM_VELODYNE_T_TYPE_DATA_PACKET) {

        int i_f = 0;

        double ctheta = VELODYNE_GET_ROT_POS(v->data, VELODYNE_DATA_FIRING_START(i_f));

        static double test_handler_ctheta_prev = 0;

        static double delta = 0;

        if(fabs(fabs(ctheta - test_handler_ctheta_prev) - delta) > 0.1 && fabs(fabs(ctheta - test_handler_ctheta_prev) - delta) < 4.0 ){
            //fprintf(stderr,"%f Different Delta : %f, %f\n", v->utime /1.0e6, fabs(ctheta - test_handler_ctheta_prev), delta);
        }
        delta = fabs(ctheta - test_handler_ctheta_prev);

        test_handler_ctheta_prev = ctheta;

        // Decode the data packet
	velodyne_laser_return_collection_t *lrc =
	velodyne_decode_data_packet(self->calib, v->data, v->datalen, v->utime);

        int ret = -1;
        //int ret = velodyne_collector_push_laser_returns (self->collector, lrc);

        velodyne_free_laser_return_collection (lrc);

        return;

        if (VELODYNE_COLLECTION_READY == ret) {
            //fprintf(stderr, "Outer count : %d\n", count);
            //count = 0;

            rate_update(self->capture_rate);

            fprintf(stderr,"Data rate : %f\n", self->capture_rate->current_hz);

            velodyne_laser_return_collection_t *lrc =
                velodyne_collector_pull_collection (self->collector);

            // if enough time has elapsed since the last scan push it onto the circular buffer

                // memory leak city if this isnt here as soon as you increase the history spacing
            velodyne_free_laser_return_collection (lrc);
        }
        else if(VELODYNE_COLLECTION_READY == ret) {
            rate_update(self->capture_rate);

            fprintf(stderr,"Data rate : %f\n", self->capture_rate->current_hz);

            velodyne_laser_return_collection_t *lrc =
                velodyne_collector_pull_collection (self->collector);

            // if enough time has elapsed since the last scan push it onto the circular buffer

                // memory leak city if this isnt here as soon as you increase the history spacing
            velodyne_free_laser_return_collection (lrc);
        }
    }
}



static int dropped_packets = 0;
static int64_t dropped_utime;

static void *
velodyne_work_thread (void *user)
{
    state_t *self = (state_t *) user;

    while (1) {

	int MAX_QUEUE_SIZE = 650;
        while (g_async_queue_length(self->velodyne_message_queue) > MAX_QUEUE_SIZE) {

	    void *msg = g_async_queue_pop (self->velodyne_message_queue);

	    // Check to see whether the thread should exit
	    g_mutex_lock (self->mutex);
	    if (self->velodyne_work_thread_exit) {
		fprintf (stdout, "got exit command\n");
		g_mutex_unlock (self->mutex);
		g_thread_exit (NULL);
	    }
	    g_mutex_unlock (self->mutex);

	    // Don't exit. Keep going
	    if (msg == &(self->velodyne_work_thread_exit))
		continue;

	    velodyne_t *v = (velodyne_t *) msg;

            velodyne_t_destroy(v);

            int64_t now = bot_timestamp_now();
            dropped_packets++;
            double dt = (now - dropped_utime) / 1000000.0;
            if (dt > 1 || dropped_packets % 1000 == 0) {
                fprintf(stderr, "WARNING: dropping velodyne packets (total dropped: %d)\n", dropped_packets);
                dropped_utime = now;
            }
        }

	void *msg = g_async_queue_pop (self->velodyne_message_queue);

	// Check to see whether the thread should exit
	g_mutex_lock (self->mutex);
	if (self->velodyne_work_thread_exit) {
	    fprintf (stdout, "got exit command\n");
	    g_mutex_unlock (self->mutex);
	    g_thread_exit (NULL);
	}
	g_mutex_unlock (self->mutex);

	// Don't exit. Keep going
        velodyne_t *v = (velodyne_t *) msg;

	self->last_msg_count = v->utime;

	process_velodyne(self, v);
	velodyne_t_destroy(v);
    }

    return NULL;
}

int
main(int argc, char ** argv)
{
    state_t *self = calloc(1,sizeof(state_t));

    self->lcm = lcm_create(NULL);//"udpm://239.255.76.67:7667?recv_buf_size=100000");
    if(!self->lcm)
        return 1;

    self->param = bot_param_get_global(self->lcm, 0);//param;
    if (!self->param) {
        fprintf (stderr,"Error: setup_renderer_laser() failed to get BotParam instance\n");
        return 1;
    }

    char key[256] = {'\0'};

    snprintf (key, sizeof(key), "%s.channel", "calibration.velodyne");
    char *lcm_channel = bot_param_get_str_or_fail (self->param, key);


    char *velodyne_model = bot_param_get_str_or_fail (self->param, "calibration.velodyne.model");
    char *calib_file = bot_param_get_str_or_fail (self->param, "calibration.velodyne.intrinsic_calib_file");

    char calib_file_path[2048];

    sprintf(calib_file_path, "%s/%s", getConfigPath(), calib_file);

    if (0 == strcmp (velodyne_model, VELODYNE_HDL_32E_MODEL_STR))
        self->calib = velodyne_calib_create (VELODYNE_SENSOR_TYPE_HDL_32E, calib_file_path);
    else if (0 == strcmp (velodyne_model, VELODYNE_HDL_64E_S1_MODEL_STR))
        self->calib = velodyne_calib_create (VELODYNE_SENSOR_TYPE_HDL_64E_S1, calib_file_path);
    else if (0 == strcmp (velodyne_model, VELODYNE_HDL_64E_S2_MODEL_STR))
        self->calib = velodyne_calib_create (VELODYNE_SENSOR_TYPE_HDL_64E_S2, calib_file_path);
    else if (0 == strcmp (velodyne_model, VELODYNE_VLP_16_MODEL_STR))
        self->calib = velodyne_calib_create (VELODYNE_SENSOR_TYPE_VLP_16, calib_file_path);
    else
        fprintf (stderr, "ERROR: Unknown Velodyne model \'%s\'", velodyne_model);

    free (velodyne_model);
    free (calib_file);

    g_thread_init (NULL);

    // Create an asynchronous queue for messages
    self->velodyne_message_queue = g_async_queue_new ();

    self->velodyne_work_thread = g_thread_create (velodyne_work_thread, self, TRUE, NULL);
    self->velodyne_work_thread_exit = 0;
    self->mutex = g_mutex_new();

    //velodyne_t_subscribe (self->lcm, lcm_channel, on_velodyne, self);
    velodyne_list_t_subscribe (self->lcm, "VELODYNE_LIST", on_velodyne_debug, self);

    self->capture_rate = rate_new();

    bot_glib_mainloop_attach_lcm (self->lcm);

    self->mainloop = g_main_loop_new (NULL, FALSE);

    bot_signal_pipe_glib_quit_on_kill (self->mainloop);

    g_main_loop_run (self->mainloop);


    // Exiting
    fprintf (stderr, "Handler exiting,\n");

    // Stop the velodyne_work thread
    g_mutex_lock (self->mutex);
    self->velodyne_work_thread_exit = 1;
    g_mutex_unlock (self->mutex);
    g_async_queue_push (self->velodyne_message_queue,
			&(self->velodyne_work_thread_exit));


    g_thread_join (self->velodyne_work_thread);

    g_mutex_free (self->mutex);

    bot_glib_mainloop_detach_lcm (self->lcm);

    int num_freed = 0;
    for (velodyne_t *msg = g_async_queue_try_pop (self->velodyne_message_queue);
	 msg; msg = g_async_queue_try_pop (self->velodyne_message_queue)) {

	if (msg == &(self->velodyne_work_thread_exit))
	    continue;

	velodyne_t_destroy(msg);
	num_freed++;
    }

    g_async_queue_unref (self->velodyne_message_queue);

    return 0;
}
