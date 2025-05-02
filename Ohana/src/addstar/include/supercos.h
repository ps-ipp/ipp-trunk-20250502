/* header for items specific to loadsupercos.c */

typedef struct {
  double longitude;
  double latitude;
  double plateScale;
  int survey_id;
} Survey;

typedef struct{
    int64_t objID;			// 0
    int8_t surveyID;			// 8
    int32_t plateID;			// 9
    double ra;				// 33
    double dec;				// 41
    double xCen;			// 129
    double yCen;			// 137
    float aU;				// 145
    float bU;				// 149
    int16_t thetaU;			// 154
    int8_t class;			// 165
    int32_t quality;			// 104
    float prfStat;			// 208
    float prfMag;			// 212
    float gMag;				// 216
    float sMag;				// 220
} Detection;

typedef struct{
    int64_t objID;			// 0
    int8_t surveyID;			// 8
    int32_t plateID;			// 9
    int64_t parentID;			// 13
    int64_t sourceID;			// 21
    int32_t recNum;			// 29
    double ra;				// 33
    double dec;				// 41
    int64_t htmId;			// 49
    double cx;				// 57
    double cy;				// 65
    double cz;				// 77
    double xmin;			// 81
    double xmax;			// 89
    double ymin;			// 97
    double ymax;			// 105
    int32_t area;			// 113
    float ipeak;			// 117
    float cosmag;			// 121
    float isky;				// 125
    double xCen;			// 129
    double yCen;			// 137
    float aU;				// 145
    float bU;				// 149
    int16_t thetaU;			// 154
    float aI;				// 155
    float bI;				// 159
    int16_t thetaI;			// 163
    int8_t class;			// 165
    int16_t pa;				// 166
    int32_t ap1;			// 168
    int32_t ap2;			// 172
    int32_t ap3;			// 176
    int32_t ap4;			// 180
    int32_t ap5;			// 184
    int32_t ap6;			// 188
    int32_t ap7;			// 192
    int32_t ap8;			// 196
    int32_t blend;			// 200
    int32_t quality;			// 204
    float prfStat;			// 208
    float prfMag;			// 212
    float gMag;				// 216
    float sMag;				// 220
    int16_t SSAfield;			// 224
    int16_t seam;			// 226 (228 total)
} FullDetection;

AddstarClientOptions args_loadsupercos (int *argc, char **argv, AddstarClientOptions options);
Survey *loadsupercos_survey (char *filename, int *nsurvey);
Image *loadsupercos_plates (Survey *survey, int Nsurvey, char *filename, int *nimage);
int loadsupercos_getFilterInfo (char *line, char *emulsion, char *filterID);
int loadsupercos_rawdata (Image *image, int *imlist, int Nimage, SkyTable *skytable, char *filename, AddstarClientOptions options);
int *loadsupercos_image_index (Image *image, int Nimage);
int loadsupercos_getST (char *line, double *st);
