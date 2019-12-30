
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>


// a structure including bboxes info
typedef struct{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
	int16_t score;
	uint16_t class;
	uint8_t alive;
}bbox_t;


typedef struct{
	float x;
	float y;
	float w;
	float h;
	int16_t score;
	uint16_t class;
	uint8_t alive;
}bbox_fp_t;

typedef struct{
	bbox_t * bbs;
	int16_t num_bb;	
}bboxs_t;

typedef struct{
	bbox_fp_t * bbs;
	int16_t num_bb;	
}bboxs_fp_t;

// with and height of default anchors
typedef struct {
	float w;
	float h;
}anchorWH_t;


// anchors layer parameters
typedef struct {
	uint16_t n_classes;
	uint8_t  feature_map_width;
	uint8_t  feature_map_height;
	uint16_t img_height;
	uint16_t img_width;
	float this_scale;
	float next_scale;
	float step_height;
	float step_width;
	float offset_height;
	float offset_width;
	float *aspect_ratios;
	uint8_t n_aspect_ratios;
	uint8_t anchor_params;
	float * variances;
	uint8_t n_variances;
	bool two_boxes_for_ar1;
	uint8_t n_anchors;
	anchorWH_t* anchorsWH;
	int* samples_per_class;
 	//int n_confidence;
 	short int confidence_thr;
 	float iou_thr;
 	//int max_candidates_per_class;
 	//int n_decoded_acnhors;
}Alps;

void SoftMax_fp16(
    short int *in_buffer,
    int i_size,
    short int *out_buffer,
    int norm //This is Qx of the input
    );


void SDD3Dto2D(short int *input, short int *output, int width, int height, int filters, int n_classes);
void SDD3Dto2DSoftmax(short int *input, short int *output, int width, int height, int filters, int n_classes,int q_in);

void PreDecoder(short int *classes, short int *boxes, Alps *anch, bboxs_fp_t* bbxs);
void PreDecoder_fixp(short int *classes, short int *boxes, Alps *anch, bboxs_t* bbxs);

void estimate_bbox(int index, float confidence, short int *Boxes, Alps *Anchors, bboxs_t *bbxs, int class);

void estimate_bbox_fp(int index, int confidence, short int *Boxes, Alps *Anchors, bboxs_fp_t *bbxs, int class);