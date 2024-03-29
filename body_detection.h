
#ifndef __BODY_DETECTION_H__
#define __BODY_DETECTION_H__

#ifdef __EMUL__
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <string.h>
#include "helpers/helpers.h"
#endif
#include "Gap.h"

extern AT_HYPERFLASH_FS_EXT_ADDR_TYPE body_detection_L3_Flash;

#endif


#if 0
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
| Step |               Name               |  In   |  Out  | Weights | Bias  | Calc  |  Acc  |
+======+==================================+=======+=======+=========+=======+=======+=======+
|  0   | input_1                          |       | Q2.14 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  1   | DEPTHWISE_CONV_2D_0_0            | Q2.14 | Q6.10 | Q5.11   | Q6.10 | Q7.25 | Q7.25 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  1   | DEPTHWISE_CONV_2D_0_0_activation | Q6.10 | Q6.10 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  1   | MAX_POOL_2D_0_1                  | Q6.10 | Q6.10 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  1   | DEPTHWISE_CONV_2D_0_0_fusion     | Q2.14 | Q6.10 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  2   | CONV_2D_0_2                      | Q6.10 | Q6.10 | Q1.15   | Q6.10 | Q7.25 | Q6.10 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  2   | CONV_2D_0_2_activation           | Q6.10 | Q6.10 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  2   | CONV_2D_0_2_fusion               | Q6.10 | Q6.10 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  3   | CONV_2D_0_4                      | Q6.10 | Q4.12 | Q1.15   | Q4.12 | Q7.25 | Q4.12 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  4   | output_1                         | Q4.12 | Q4.12 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  5   | CONV_2D_0_5                      | Q6.10 | Q5.11 | Q1.15   | Q5.11 | Q7.25 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  6   | output_5                         | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  7   | MAX_POOL_2D_0_3                  | Q6.10 | Q6.10 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  8   | CONV_2D_0_6                      | Q6.10 | Q5.11 | Q1.15   | Q5.11 | Q7.25 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  8   | CONV_2D_0_6_activation           | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  8   | CONV_2D_0_6_fusion               | Q6.10 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  9   | CONV_2D_0_8                      | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  10  | output_2                         | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  11  | CONV_2D_0_9                      | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  12  | output_6                         | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  13  | MAX_POOL_2D_0_7                  | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  14  | CONV_2D_0_10                     | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  14  | CONV_2D_0_10_activation          | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  14  | CONV_2D_0_10_fusion              | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  15  | CONV_2D_0_12                     | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  16  | output_3                         | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  17  | CONV_2D_0_13                     | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  18  | output_7                         | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  19  | MAX_POOL_2D_0_11                 | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  20  | CONV_2D_0_14                     | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  20  | CONV_2D_0_14_activation          | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  20  | CONV_2D_0_14_fusion              | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  21  | CONV_2D_0_15                     | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  22  | output_4                         | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  23  | CONV_2D_0_16                     | Q5.11 | Q5.11 | Q1.15   | Q5.11 | Q6.26 | Q5.11 |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+
|  24  | output_8                         | Q5.11 | Q5.11 |         |       |       |       |
+------+----------------------------------+-------+-------+---------+-------+-------+-------+

#endif //__BODY_DETECTION_H__
