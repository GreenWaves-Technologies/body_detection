/*
 * Copyright (C) 2017 GreenWaves Technologies
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 *
 */

#include <stdio.h>

#include "setup.h"
#include "Gap.h"
#include "body_detectionKernels.h"
#include "SSDKernels.h"
#include "SSDParams.h"
#include "pmsis.h"
#include "bsp/display/ili9341.h"
#include "bsp/camera/himax.h"

#ifdef __EMUL__
#ifdef PERF
#undef PERF
#endif
#endif

#if SILENT
#define PRINTF(...) ((void) 0)
#else
#define PRINTF printf
#endif


struct pi_device ili;
struct pi_device device;
static pi_buffer_t buffer;

//TODO: how what are these parameters
#define STACK_SIZE           2*1024 //This is for PE0   (Master)
#define SLAVE_STACK_SIZE     1024 //This is for PE1-7 (Slaves)
#define MOUNT           1
#define UNMOUNT         0
#define CID             0

struct pi_device HyperRam;
static struct pi_hyperram_conf conf;

AT_HYPERFLASH_FS_EXT_ADDR_TYPE body_detection_L3_Flash = 0;

#ifdef __EMUL__
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <string.h>
#include "helpers/helpers.h"
//To be integrated in AT Header

#ifndef TENSOR_DUMP_FILE
#define TENSOR_DUMP_FILE "tensor_dump_file.dat"
#endif
#endif

#define FIX2FP(Val, Precision)    ((float) (Val) / (float) (1<<(Precision)))

//static short int *L2_Output_1, L2_Output_2, L2_Output_3, L2_Output_4, L2_Output_5, L2_Output_6, L2_Output_7, L2_Output_8;
//PI_L2 short int *L2_Output_1, *L2_Output_2, *L2_Output_3, *L2_Output_4, *L2_Output_5, *L2_Output_6, *L2_Output_7, *L2_Output_8;


PI_L2 short int *tmp_buffer_classes, *tmp_buffer_boxes;

typedef short int MNIST_IMAGE_IN_T;

L2_MEM MNIST_IMAGE_IN_T *ImageIn;

extern PI_L2 Alps * anchor_layer_1;
extern PI_L2 Alps * anchor_layer_2;
extern PI_L2 Alps * anchor_layer_3;
extern PI_L2 Alps * anchor_layer_4;


short int * Output_1; 
short int * Output_2; 
short int * Output_3; 
short int * Output_4; 
short int * Output_5; 
short int * Output_6; 
short int * Output_7; 
short int * Output_8;

#ifdef __EMUL__
PI_L2 bboxs_fp_t bbxs;
#else
PI_L2 bboxs_t bbxs;
#endif
static int initSSD(){

    #ifdef __EMUL__
    bbxs.bbs = pmsis_l2_malloc(sizeof(bbox_fp_t)*MAX_BB);
    #else
    bbxs.bbs = pmsis_l2_malloc(sizeof(bbox_t)*MAX_BB);
    #endif

    if(bbxs.bbs==NULL){
        printf("Bounding Boxes Allocation Error...\n");
        return 1;
    }

    bbxs.num_bb = 0;


    initAnchorLayer_1();
    initAnchorLayer_2();
    initAnchorLayer_3();
    initAnchorLayer_4();

    return 0;

}


void convertCoordBboxes(bboxs_t *boundbxs,float scale_x,float scale_y){

    for (int counter=0;counter< boundbxs->num_bb;counter++){
        //Set Alive for non max this should be done somewhere else
        boundbxs->bbs[counter].alive=1;
        boundbxs->bbs[counter].x = (int)(FIX2FP(boundbxs->bbs[counter].x,20) * scale_x);
        boundbxs->bbs[counter].y = (int)(FIX2FP(boundbxs->bbs[counter].y,20) * scale_y);
        boundbxs->bbs[counter].w = (int)(FIX2FP(boundbxs->bbs[counter].w,26) * scale_x);
        boundbxs->bbs[counter].h = (int)(FIX2FP(boundbxs->bbs[counter].h,26) * scale_y);
        boundbxs->bbs[counter].x = boundbxs->bbs[counter].x - (boundbxs->bbs[counter].w/2);
        boundbxs->bbs[counter].y = boundbxs->bbs[counter].y - (boundbxs->bbs[counter].h/2);
    }
}


void printBboxes(bboxs_t *boundbxs){
    PRINTF("\n\n======================================================");
    PRINTF("\nDetected Bounding boxes                                 ");
    PRINTF("\n======================================================\n");
    PRINTF("BoudingBox:  score     cx     cy     w     h    class");
    PRINTF("\n------------------------------------------------------\n");

    for (int counter=0;counter< boundbxs->num_bb;counter++){
        if(boundbxs->bbs[counter].alive)
            PRINTF("bbox [%02d] : %.5f     %03d    %03d     %03d    %03d     %02d\n",
                counter,
                FIX2FP(boundbxs->bbs[counter].score,15 ),
                boundbxs->bbs[counter].x,
                boundbxs->bbs[counter].y,
                boundbxs->bbs[counter].w,
                boundbxs->bbs[counter].h,
                boundbxs->bbs[counter].class);
    }//
}


void printBboxes_forPython(bboxs_t *boundbxs){
    PRINTF("\n\n======================================================");
    PRINTF("\nThis can be copy-pasted to python to draw BoudingBoxs   ");
    PRINTF("\n\n");

    for (int counter=0;counter< boundbxs->num_bb;counter++){
        if(boundbxs->bbs[counter].alive)
            PRINTF("rect = patches.Rectangle((%d,%d),%d,%d,linewidth=1,edgecolor='r',facecolor='none')\nax.add_patch(rect)\n",
                boundbxs->bbs[counter].x,
                boundbxs->bbs[counter].y,
                boundbxs->bbs[counter].w,
                boundbxs->bbs[counter].h);
    }//
}


#define NON_MAX_THRES 10

int rect_intersect_area( short a_x, short a_y, short a_w, short a_h,
                         short b_x, short b_y, short b_w, short b_h ){

    #define MIN(a,b) ((a) < (b) ? (a) : (b))
    #define MAX(a,b) ((a) > (b) ? (a) : (b))

    int x = MAX(a_x,b_x);
    int y = MAX(a_y,b_y);

    int size_x = MIN(a_x+a_w,b_x+b_w) - x;
    int size_y = MIN(a_y+a_h,b_y+b_h) - y;

    if(size_x <=0 || size_x <=0)
        return 0;
    else
        return size_x*size_y; 

    #undef MAX
    #undef MIN
}


void non_max_suppress(bboxs_t * boundbxs){

    int idx,idx_int;

    //Non-max supression
     for(idx=0;idx<boundbxs->num_bb;idx++){
        //check if rect has been removed (-1)
        if(boundbxs->bbs[idx].alive==0)
            continue;
 
        for(idx_int=0;idx_int<boundbxs->num_bb;idx_int++){

            if(boundbxs->bbs[idx_int].alive==0 || idx_int==idx)
                continue;

            //check the intersection between rects
            int intersection = rect_intersect_area(boundbxs->bbs[idx].x,boundbxs->bbs[idx].y,boundbxs->bbs[idx].w,boundbxs->bbs[idx].h,
                                                   boundbxs->bbs[idx_int].x,boundbxs->bbs[idx_int].y,boundbxs->bbs[idx_int].w,boundbxs->bbs[idx_int].h);

            if(intersection >= NON_MAX_THRES){ //is non-max
                //supress the one that has lower score that is alway the internal index, since the input is sorted
                boundbxs->bbs[idx_int].alive =0;
            }
        }
    }
}


static void RunNN()
{

    unsigned int ti,ti_nn,ti_ssd;

    gap_cl_starttimer();
    gap_cl_resethwtimer();
    ti = gap_cl_readhwtimer();

    body_detectionCNN(ImageIn, Output_1, Output_2, Output_3, Output_4, Output_5, Output_6, Output_7, Output_8);

    ti_nn = gap_cl_readhwtimer()-ti;
    PRINTF("Cycles NN : %10d\n",ti_nn);
}

    /////////////////////
    //SSD Code
    /////////////////////

static void RunSSD()
{

    unsigned int ti,ti_ssd;

    gap_cl_resethwtimer();
    ti = gap_cl_readhwtimer();

    //Set Boundinx Boxes to 0
    bbxs.num_bb = 0;
    
    //TODO Quantization is likely wrong need to check output

    SDD3Dto2DSoftmax_80_60_12(Output_1,tmp_buffer_classes,13,2);
    SDD3Dto2D_80_60_24(Output_5,tmp_buffer_boxes,0,0);
    Predecoder80_60(tmp_buffer_classes, tmp_buffer_boxes, anchor_layer_1, &bbxs,11);

    SDD3Dto2DSoftmax_40_30_14(Output_2,tmp_buffer_classes,12,2);
    SDD3Dto2D_40_30_28(Output_6,tmp_buffer_boxes,0,0);
    Predecoder40_30(tmp_buffer_classes, tmp_buffer_boxes, anchor_layer_2, &bbxs,12);
    
    SDD3Dto2DSoftmax_20_15_16(Output_3,tmp_buffer_classes,13,2);
    SDD3Dto2D_20_15_32(Output_7,tmp_buffer_boxes,0,0);
    Predecoder20_15(tmp_buffer_classes, tmp_buffer_boxes, anchor_layer_3,&bbxs,12);
    
    SDD3Dto2DSoftmax_10_7_14(Output_4,tmp_buffer_classes,13,2);
    SDD3Dto2D_10_7_28(Output_8,tmp_buffer_boxes,0,0);
    Predecoder10_7(tmp_buffer_classes, tmp_buffer_boxes, anchor_layer_4, &bbxs,12);


    bbox_t temp;

    int changed=0;
    do{
        changed=0;
        for(int i=0; i<bbxs.num_bb-1;i++){
            if(bbxs.bbs[i].score < bbxs.bbs[i+1].score){
                temp          = bbxs.bbs[i];
                bbxs.bbs[i]   = bbxs.bbs[i+1];
                bbxs.bbs[i+1] = temp;
                changed=1;
            }
        }
    }while(changed);

    //The signature of the function should be changed because the input image is not square
    //TODO!!!!!!
    convertCoordBboxes(&bbxs,160,120); 
    non_max_suppress(&bbxs);

    ti_ssd = gap_cl_readhwtimer()-ti;

    printBboxes(&bbxs);
    printBboxes_forPython(&bbxs);
    
    PRINTF("Cycles SSD: %10d\n",ti_ssd);

}

static int open_display(struct pi_device *device)
{
  struct pi_ili9341_conf ili_conf;

  pi_ili9341_conf_init(&ili_conf);

  pi_open_from_conf(device, &ili_conf);

  if (pi_display_open(device))
    return -1;

  if (pi_display_ioctl(device, PI_ILI_IOCTL_ORIENTATION, (void *)PI_ILI_ORIENTATION_180))
    return -1;

  return 0;
}


static int open_camera_himax(struct pi_device *device)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;

  return 0;
}

void drawBboxes(bboxs_t *boundbxs, uint8_t *img){

     for (int counter=0;counter< boundbxs->num_bb;counter++){
        if(boundbxs->bbs[counter].alive){
            DrawRectangle(img, 120, 160, boundbxs->bbs[counter].x, boundbxs->bbs[counter].y, boundbxs->bbs[counter].w, boundbxs->bbs[counter].h, 255);
        }
    }
}

int checkResults(bboxs_t *boundbxs){
    int totAliveBB=0;
    int x,y,w,h;

    for (int counter=0;counter< boundbxs->num_bb;counter++){
        if(boundbxs->bbs[counter].alive){
            totAliveBB++;
            x = boundbxs->bbs[counter].x;
            y = boundbxs->bbs[counter].y;
            w = boundbxs->bbs[counter].w;
            h = boundbxs->bbs[counter].h;
        }
    }

    //Cabled check of result
    if(totAliveBB!=1) return -1;
    if( x != 74 )         return -1;
    if( y != 26 )         return -1;
    if( w != 28 )         return -1;
    if( h != 79 )         return -1;

    return 0;

}


#ifdef __EMUL__
int main(int argc, char *argv[])
{

	if (argc < 2)
    {
        printf("Usage: mnist [image_file]\n");
        exit(1);
    }
    char *ImageName = argv[1];
#else

int main()
{
    //char *ImageName = "../../../test_samples/img_OUT0.pgm";
    char *ImageName = "../../../samples/test_people.pgm";
    

#endif
    unsigned int Wi, Hi;
    //Input image size
    unsigned int W = 160, H = 120;
    unsigned int Wcam=324, Hcam=244;


    PRINTF("Entering main controller\n");

    if (rt_event_alloc(NULL, 16)) return -1;
    pi_freq_set(PI_FREQ_DOMAIN_FC,150000000);

#ifdef FROM_CAMERA

    unsigned char *ImageInChar = (unsigned char *) pmsis_l2_malloc( Wcam * Hcam * sizeof(unsigned char));
    if (ImageInChar == 0)
    {
        printf("Failed to allocate Memory for Image (%d bytes)\n", Wcam * Hcam * sizeof(MNIST_IMAGE_IN_T));
        pmsis_exit(-6);
    }
    ImageIn = (MNIST_IMAGE_IN_T *)ImageInChar;

    if (open_display(&ili))
    {
        printf("Failed to open display\n");
        pmsis_exit(-1);
    }

    buffer.data = ImageInChar;
    buffer.stride = 0;

    // WIth Himax, propertly configure the buffer to skip boarder pixels
    pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, ImageInChar);
    pi_buffer_set_stride(&buffer, 0);
    pi_buffer_set_format(&buffer, 160, 120, 1, PI_BUFFER_FORMAT_GRAY);


    if (open_camera_himax(&device))
    {
        printf("Failed to open camera\n");
        pmsis_exit(-2);
    }


#else

    unsigned char *ImageInChar = (unsigned char *) pmsis_l2_malloc( W * H * sizeof(MNIST_IMAGE_IN_T));
    if (ImageInChar == 0)
    {
        printf("Failed to allocate Memory for Image (%d bytes)\n", W * H * sizeof(MNIST_IMAGE_IN_T));
        pmsis_exit(-6);
    }

    #ifndef NO_BRIDGE
    //Reading Image from Bridge
    PRINTF("Loading Image from File\n");
    if ((ReadImageFromFile(ImageName, &Wi, &Hi, ImageInChar, W * H * sizeof(unsigned char)) == 0) || (Wi != W) || (Hi != H))
    {
        printf("Failed to load image %s or dimension mismatch Expects [%dx%d], Got [%dx%d]\n", ImageName, W, H, Wi, Hi);
        pmsis_exit(-6);
    }
    #endif

    ImageIn = (MNIST_IMAGE_IN_T *)ImageInChar;


    #ifndef NO_BRIDGE

    for (int i = W * H - 1; i >= 0; i--)
    {
        ImageIn[i] = ImageInChar[i] << 6; //Input is Q14
    }

    #else
    for (int i = W * H - 1; i >= 0; i--)
    {
        ImageIn[i] = inImage[i] << 6; //Input is Q14
    }
    #endif
#endif


    /* Init & open ram. */
    pi_hyperram_conf_init(&conf);
    pi_open_from_conf(&HyperRam, &conf);
    if (pi_ram_open(&HyperRam))
    {
        printf("Error ram open !\n");
        pmsis_exit(-5);
    }

    pi_ram_alloc(&HyperRam, &Output_1, 60 * 80* 12 * sizeof(short int));
    pi_ram_alloc(&HyperRam, &Output_2, 30 * 40* 14 * sizeof(short int));
    pi_ram_alloc(&HyperRam, &Output_3, 15 * 20* 16 * sizeof(short int));
    pi_ram_alloc(&HyperRam, &Output_4, 7  * 10* 14 * sizeof(short int));
    
    pi_ram_alloc(&HyperRam, &Output_5, 60 * 80* 24 * sizeof(short int));
    pi_ram_alloc(&HyperRam, &Output_6, 30 * 40* 28 * sizeof(short int));
    pi_ram_alloc(&HyperRam, &Output_7, 15 * 20* 32 * sizeof(short int));
    pi_ram_alloc(&HyperRam, &Output_8, 7  * 10* 28 * sizeof(short int));

    pi_ram_alloc(&HyperRam, &tmp_buffer_classes, 60 * 80* 12   * sizeof(short int));
    pi_ram_alloc(&HyperRam, &tmp_buffer_boxes  , 60 * 80* 24   * sizeof(short int));

    if(Output_1==NULL || Output_2==NULL || Output_3==NULL || Output_4==NULL || Output_5==NULL || Output_6==NULL || Output_7==NULL || Output_8==NULL )
    {
        printf("Error Allocating OUTPUTs in L3\n");
        pmsis_exit(-7);
    }

    if(tmp_buffer_classes==NULL || tmp_buffer_classes==NULL)
    {
        printf("Error Allocating SSD Temp buffers in L3\n");
        pmsis_exit(-7);
    }

    #ifndef __EMUL__
    /* Configure And open cluster. */
    struct pi_device cluster_dev;
    struct pi_cluster_conf cl_conf;
    cl_conf.id = 0;
    pi_open_from_conf(&cluster_dev, (void *) &cl_conf);
    if (pi_cluster_open(&cluster_dev))
    {
        printf("Cluster open failed !\n");
        pmsis_exit(-7);
    }

    #endif

    if(initSSD())
    {
        printf("NN Init exited with an error\n");
        pmsis_exit(-6);
    }
    
    PRINTF("Running NN\n");

    struct pi_cluster_task *task = pmsis_l2_malloc(sizeof(struct pi_cluster_task));
    if(task==NULL) {
        printf("Alloc Error! \n");
        pmsis_exit(-5);
    }
    
    int iter=1;

    pi_freq_set(PI_FREQ_DOMAIN_CL,175000000);

    while(iter){

        #ifndef FROM_CAMERA
        iter=0;
        #else
            pi_camera_control(&device, PI_CAMERA_CMD_START, 0);
            pi_camera_capture(&device, ImageInChar, Wcam*Hcam);
            pi_camera_control(&device, PI_CAMERA_CMD_STOP, 0);
            int Xoffset = (Wcam - 160)/2;
            int Yoffset = (Hcam - 120)/2;
            for(int y=0;y<120;y++){
                for(int x=0;x<160;x++){
                    ImageIn[y*160+x] = ((short int)ImageInChar[((y+Yoffset)*Wcam)+(x+Xoffset)]) << 6;
                }
            }
        #endif

        if (body_detectionCNN_Construct())
        {
            printf("Graph constructor exited with an error\n");
            pmsis_exit(-4);
        }

        memset(task, 0, sizeof(struct pi_cluster_task));
        task->entry = RunNN;
        task->arg = (void *) NULL;
        task->stack_size = (uint32_t) STACK_SIZE;
        task->slave_stack_size = (uint32_t) SLAVE_STACK_SIZE;
    

        pi_cluster_send_task_to_cl(&cluster_dev, task);
        #ifndef FROM_CAMERA
        {
            unsigned int TotalCycles = 0, TotalOper = 0;
            printf("\n");
            for (unsigned int i=0; i<(sizeof(NNPerf)/sizeof(unsigned int)); i++)
            {
                printf("%45s: %10d, Operation: %10d, Operation/Cycle: %f\n", NNLName[i], NNPerf[i], NNOperCount[i], ((float) NNOperCount[i])/ NNPerf[i]);
                TotalCycles += NNPerf[i]; TotalOper += NNOperCount[i];
            }
            printf("\n");
            printf("%45s: %10d, Operation: %10d, Operation/Cycle: %f\n", "Total", TotalCycles, TotalOper, ((float) TotalOper)/ TotalCycles);
            printf("\n");
        }
        #endif  /* FROM_CAMERA */

        body_detectionCNN_Destruct();

        //SSD Allocations
        SSDKernels_L1_Memory = pmsis_l1_malloc(_SSDKernels_L1_Memory_SIZE);
        SSDKernels_L2_Memory = pmsis_l2_malloc(_SSDKernels_L2_Memory_SIZE);

        if(SSDKernels_L1_Memory==NULL || SSDKernels_L2_Memory==NULL)
        {
            printf("SSD L1 and/or L2 allocation error\n");
            pmsis_exit(-3);
        }


        memset(task, 0, sizeof(struct pi_cluster_task));
        task->entry = RunSSD;
        task->arg = (void *) NULL;
        task->stack_size = (uint32_t) STACK_SIZE;
        task->slave_stack_size = (uint32_t) SLAVE_STACK_SIZE;
        pi_cluster_send_task_to_cl(&cluster_dev, task);

        pmsis_l1_malloc_free(SSDKernels_L1_Memory,_SSDKernels_L1_Memory_SIZE);
        pmsis_l2_malloc_free(SSDKernels_L2_Memory,_SSDKernels_L2_Memory_SIZE);

        #ifdef FROM_CAMERA
        for(int y=0;y<120;y++){
            for(int x=0;x<160;x++){
                ImageInChar[y*160+x] = (unsigned char)(ImageIn[(y*160)+(x)] >> 6);
            }
        }
        //Draw BBs
        drawBboxes(&bbxs,ImageInChar);
        //Send to Screen
        pi_display_write(&ili, &buffer, 60, 80, 160, 120);
        #endif
    }

#ifndef __EMUL__
    // Close the cluster
    pi_cluster_close(&cluster_dev);
#endif


#ifdef __EMUL__
    dt_close_dump_file();
#else
#endif

    PRINTF("Ended\n");
    
    if(checkResults(&bbxs)==0){
        printf("Correct results!\n");
        return 0;
    }
    else{
        printf("Wrong results!\n");
        return -1;
    }


    return 0;
}
