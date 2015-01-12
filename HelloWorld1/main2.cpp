/***************************************************************************/
/*                                                                         */
/* 234218 Data DSs 1, Winter 2014-2015                                          */
/*                                                                         */
/* Homework : Wet 2                                                        */
/*                                                                         */
/***************************************************************************/

/***************************************************************************/
/*                                                                         */
/* File Name : main2.cpp                                                   */
/*                                                                         */
/* Holds the "int main()" function and the parser of the shell's           */
/* command line.                                                           */
/***************************************************************************/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "library2.h"
#include <cstring>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif




/* The command's strings */
typedef enum {
  NONE_CMD = -2,
  COMMENT_CMD = -1,
  INIT_CMD = 0,
  ADDCITIZEN_CMD = 1,
  MOVETOCITY_CMD = 2,
  JOINKINGDOMS_CMD = 3,
  GETCAPITAL_CMD = 4,
  SELECTCITY_CMD = 5,
  GETCITIES_CMD = 6,
  QUIT_CMD = 7
} commandType;



static const int   numActions   = 8;
static const char *commandStr[] = {
  "Init",
  "AddCitizen",
  "MoveToCity",
  "JoinKingdoms",
  "GetCapital",
  "SelectCity",
  "GetCitiesBySize",
  "Quit"
};


static const char* ReturnValToStr(int val) {
	switch (val) {
		case (SUCCESS):          return "Success";
		case (FAILURE):          return "Failure";
		case (ALLOCATION_ERROR): return "Allocation_error";
		case (INVALID_INPUT):    return "Invalid_input";
		default:                 return "";
	}
}
	





/* we assume maximum string size is not longer than 256  */
#define MAX_STRING_INPUT_SIZE (255)
#define MAX_BUFFER_SIZE       (255)

#define StrCmp(Src1,Src2) ( strncmp((Src1),(Src2),strlen(Src1)) == 0 )

typedef enum {error_free, error} errorType;
static errorType parser(const char* const command);



#define ValidateRead(read_parameters,required_parameters,ErrorString) \
if ( (read_parameters)!=(required_parameters) ) { printf(ErrorString); return error; }


static bool isInit = false;


/* Print an array */
string PrintIntArray(const int* arr, int size) {
	char buffer[MAX_BUFFER_SIZE];
	string str = "";

	for (int i=0; i < size; i++) {
		sprintf(buffer,"%d",arr[i]);
		str += string(buffer) + ((i == (size - 1)) ? "" : ",");
	}
	return str.c_str();
}




/***************************************************************************/
/* main                                                                    */
/***************************************************************************/

int main(int argc, const char**argv) {
  char buffer[MAX_STRING_INPUT_SIZE];
  // Reading commands
  while ( fgets(buffer, MAX_STRING_INPUT_SIZE, stdin) != NULL ) {
    fflush(stdout); 
    if ( parser(buffer) == error )
      break;
  };
  return 0;
};

/***************************************************************************/
/* Command Checker                                                         */
/***************************************************************************/

static commandType CheckCommand(const char* const command, const char** const command_arg) {
  if ( command == NULL || strlen(command) == 0 || StrCmp("\n", command) )
    return(NONE_CMD);
  if ( StrCmp("#", command) ) {
    if (strlen(command) > 1)
      printf("%s", command);
    return(COMMENT_CMD);
  };
  for (int index=0; index < numActions; index++) {
    if ( StrCmp(commandStr[index], command) ) {
      *command_arg = command + strlen(commandStr[index]) + 1;
      return((commandType)index);
    };
  };
  return(NONE_CMD);
};

/***************************************************************************/
/* Commands Functions                                                      */
/***************************************************************************/

static errorType OnInit(void** DS, const char* const command);
static errorType OnAddCitizen(void* DS, const char* const command);
static errorType OnMoveToCity(void* DS, const char* const command);
static errorType OnJoinKingdoms(void* DS, const char* const command);
static errorType OnGetCapital(void* DS, const char* const command);
static errorType OnSelectCity(void* DS, const char* const command);
static errorType OnGetCitiesBySize(void* DS, const char* const command);
static errorType OnQuit(void** DS, const char* const command);




/***************************************************************************/
/* Parser                                                                  */
/***************************************************************************/

static errorType parser(const char* const command) { 
  static void *DS = NULL; /* The general data structure */
  const char* command_args = NULL;
  errorType rtn_val = error;

  commandType command_val = CheckCommand(command, &command_args);
 
  switch (command_val) {

	case (INIT_CMD):                   	rtn_val = OnInit(&DS, command_args);	break;
	case (ADDCITIZEN_CMD):             	rtn_val = OnAddCitizen(DS, command_args);	break;
	case (MOVETOCITY_CMD):         		rtn_val = OnMoveToCity(DS, command_args);	break;
	case (JOINKINGDOMS_CMD):          	rtn_val = OnJoinKingdoms(DS, command_args);	break;
	case (GETCAPITAL_CMD):             	rtn_val = OnGetCapital(DS, command_args);	break;
	case (SELECTCITY_CMD):             	rtn_val = OnSelectCity(DS, command_args);	break;
	case (GETCITIES_CMD):         		rtn_val = OnGetCitiesBySize(DS, command_args);	break;
	case (QUIT_CMD):                   	rtn_val = OnQuit(&DS, command_args);	break;
	
	case (COMMENT_CMD):                	rtn_val = error_free;	break;
	case (NONE_CMD):                   	rtn_val = error;	break;
	default: assert(false);
  };
  return(rtn_val);
};



int INIT_n;


/***************************************************************************/
/* OnInit                                                                  */
/***************************************************************************/
static errorType OnInit(void** DS, const char* const command) {
	if(isInit) {
		printf("Init was already called.\n");
		return(error_free);
	};
	isInit = true;

	ValidateRead( sscanf(command, "%d" ,&INIT_n), 1, "Init failed.\n" );
		
	*DS = Init(INIT_n);
	if( *DS == NULL ) {
		printf("Init failed.\n");
		return(error);
	};
	printf("Init done.\n");

	return error_free;
}


/***************************************************************************/
/* OnAddCitizen                                                           	*/
/***************************************************************************/
static errorType OnAddCitizen(void* DS, const char* const command) {
	int citizenID;
	ValidateRead( sscanf(command, "%d",&citizenID), 1, "AddCitizen failed.\n" );
	StatusType res = AddCitizen(DS,citizenID);
	
	printf("AddCitizen: %s\n", ReturnValToStr(res));
	
	return error_free;
}


/***************************************************************************/
/* OnMoveToCity                                                                  */
/***************************************************************************/
static errorType OnMoveToCity(void* DS, const char* const command) {
	int citizenID;
	int city;
	ValidateRead( sscanf(command, "%d %d",&citizenID,&city), 2, "MoveToCity failed.\n" );
	StatusType res = MoveToCity(DS,citizenID,city);

	printf("MoveToCity: %s\n", ReturnValToStr(res));

	return error_free;
}


/***************************************************************************/
/* OnJoinKingdoms                                                         */
/***************************************************************************/
static errorType OnJoinKingdoms(void* DS, const char* const command) {
	int city1;
	int city2;
	ValidateRead( sscanf(command, "%d %d",&city1,&city2), 2, "JoinKingdoms failed.\n" );
	StatusType res = JoinKingdoms(DS,city1,city2);
	
	printf("JoinKingdoms: %s\n", ReturnValToStr(res));
	
	return error_free;
}


/***************************************************************************/
/* OnGetCapital                                                            */
/***************************************************************************/
static errorType OnGetCapital(void* DS, const char* const command) {
	int citizenID;
	ValidateRead( sscanf(command, "%d",&citizenID), 1, "GetCapital failed.\n" );
	int	capital;
	StatusType res = GetCapital(DS,citizenID,&capital);
	
	if (res != SUCCESS) {
		printf("GetCapital: %s\n",ReturnValToStr(res));
	}
	else {
		printf("GetCapital: %s %d\n", ReturnValToStr(res),capital);
	}

	return error_free;
}


/***************************************************************************/
/* OnSelectCity                                                            */
/***************************************************************************/
static errorType OnSelectCity(void* DS, const char* const command) {
	int k;
	ValidateRead( sscanf(command, "%d",&k), 1, "SelectCity failed.\n" );
	int	city;
	StatusType res = SelectCity(DS,k,&city);

	if (res != SUCCESS) {
		printf("SelectCity: %s\n",ReturnValToStr(res));
	}
	else {
		printf("SelectCity: %s %d\n", ReturnValToStr(res),city);
	}

	return error_free;
}


/***************************************************************************/
/* OnGetCitiesBySize                                                        */
/***************************************************************************/
static errorType OnGetCitiesBySize(void* DS, const char* const command) {
	ValidateRead( sscanf(command, " "), 0, "GetCitiesBySize failed.\n" );
	
	int results[INIT_n];
	StatusType res = GetCitiesBySize(DS, results);

	if (res != SUCCESS) {
		printf("GetCitiesBySize: %s\n",ReturnValToStr(res));
	}
	else {
		printf("GetCitiesBySize: %s, ", ReturnValToStr(res));
		for (int i=0; i < INIT_n; i++) {
			printf("%d", results[i]);
			if (i < (INIT_n - 1)) {
				printf(",");
			}
		}
		printf("\n");
	}

	return error_free;
}


/***************************************************************************/
/* OnQuit                                                                  */
/***************************************************************************/
static errorType OnQuit(void** DS, const char* const command) {
	Quit(DS);
	if( *DS != NULL ) {
		printf("Quit failed.\n");
		return(error);
	};
	isInit = false;
	printf("Quit done.\n");

	return error_free;
}




#ifdef __cplusplus
}
#endif


