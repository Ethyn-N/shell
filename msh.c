// The MIT License (MIT)
// 
// Copyright (c) 2016 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments

int updateHistory(char history[][MAX_COMMAND_SIZE], int history_index, char *command_string);

int main() 
{

  char *command_string = (char*) malloc( MAX_COMMAND_SIZE );
 
  char history[15][MAX_COMMAND_SIZE] = { 0 };
  int history_index = 0;
    
  while (1) 
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(command_string, MAX_COMMAND_SIZE, stdin));

    // If command line input has '!', find corresponding history if there is one
    if (command_string[0] == '!')
    {
      char temp[MAX_COMMAND_SIZE];
      strncpy(temp, command_string + 1, strlen(command_string) - 2);
      int command_number = atoi(temp);
      if (command_number <= 0 || command_number >= history_index)
      {
        printf("Command not in history.\n");
        history_index = updateHistory(history, history_index, command_string);
        continue;
      }
      strcpy(command_string, history[command_number]);
    }

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
    {
      token[i] = NULL;
    }

    int token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string = strdup(command_string);                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this for loop and replace with your shell functionality

    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }

    if (token[0] == NULL)
    {
      continue;
    }

    if ((strcmp("quit", token[0]) == 0) || (strcmp("exit", token[0]) == 0))
    {
      exit(0);
    }
    else if (strcmp("cd", token[0]) == 0)
    {
      if (chdir(token[1]) == -1)
      {
        printf("%s: Directory not found.\n", token[1]);
      }
      history_index = updateHistory(history, history_index, command_string);
      
    }
    else if (strcmp("history", token[0]) == 0)
    {
      history_index = updateHistory(history, history_index, command_string);

      for(int i = 0; i < history_index; i++)
      {
        printf("%d: %s", i, history[i]);
      }
    }
    else
    {
      pid_t pid = fork();

      if (pid == -1) // Process failed
      {
        perror("fork failed: ");
        exit(1);
      }

      if (pid == 0) // Child process
      {
        int ret = execvp(token[0], token);
        if (ret == -1)
        {
          printf("%s: Command not found.\n", token[0]);
        }
        //Exit child process before the parent process.
        exit(1);
      }
      else // Parent process
      {
        int status;
        waitpid(pid, &status, 0);
        history_index = updateHistory(history, history_index, command_string);
        fflush(NULL);
      }
    }

    // Cleanup allocated memory
    for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
    {
      if (token[i] != NULL)
      {
        free(token[i]);
      }
    }

    free(head_ptr);

  }

  free(command_string);

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}

// Updates history array with most recent commands
int updateHistory(char history[][MAX_COMMAND_SIZE], int history_index, char *command_string)
{
  if (history_index == 15)
  {  
    for (int i = 1; i < 15; i++)
    {
      strcpy(history[i-1], history[i]);
    }
    strcpy(history[14], command_string);
  }
  else
  {
    strcpy(history[history_index], command_string);
    history_index++;
  }

  return history_index;
}
