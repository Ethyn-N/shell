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
                                // In this case white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments

#define MAX_HISTORY_SIZE 15 // The maximum history size

#define MAX_PID_SIZE 15 // The maximum pids size

int updateHistory(char history[][MAX_COMMAND_SIZE], int history_index, char *command_string);
int updatePids(int pids[MAX_PID_SIZE], int pids_index, int pid);

int main() 
{
  char *command_string = (char*) malloc(MAX_COMMAND_SIZE);
 
  char history[MAX_HISTORY_SIZE][MAX_COMMAND_SIZE] = { 0 };
  int history_index = 0;

  int pids[MAX_PID_SIZE] = { 0 };
  int pids_index = 0;
    
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

    // If the command line input has '!' as the first character, 
    // find the corresponding history if there is one
    // and overwrite command_str with the command in history.
    if (command_string[0] == '!')
    {
      char temp[MAX_COMMAND_SIZE];

      // Copy rest of command_string without '!' into temp.
      strncpy(temp, command_string + 1, strlen(command_string) - 2); 

      // Find the commmand number given and look if it is in history.
      // If not in history, print not in history, upadate history, and continue.
      // Otherwise, overwrite command_str with the command in history.
      int command_number = atoi(temp);
      if (command_number < 0 || command_number >= history_index)
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

    // Continue if user inputs enters a blank line.
    if (token[0] == NULL)
    {
      continue;
    }

    // Program exits if "quit" or "exit" command inoked.
    if ((strcmp("quit", token[0]) == 0) || (strcmp("exit", token[0]) == 0))
    {
      exit(0);
    }

    // If valid directory exists, chdir and update history.
    else if (strcmp("cd", token[0]) == 0)
    {
      if (chdir(token[1]) == -1)
      {
        printf("%s: Directory not found.\n", token[1]);
      }
      history_index = updateHistory(history, history_index, command_string);
    }

    // If "history" command is invoked without parameters,
    // update history and list the last 15 commands entered by the user.
    else if (strcmp("history", token[0]) == 0 && token[1] == NULL)
    {
      history_index = updateHistory(history, history_index, command_string);

      for(int i = 0; i < history_index; i++)
      {
        printf("%d: %s", i, history[i]);
      }
    }
    else if (strcmp("history", token[0]) == 0 && token[1] != NULL)
    {
      if (strcmp("-p", token[1]) != 0)
      {
        printf("%s: invalid option -- '%s'\n", token[0], token[1]);
        history_index = updateHistory(history, history_index, command_string);
        continue;
      }

      history_index = updateHistory(history, history_index, command_string);

      for(int i = 0; i < pids_index; i++)
      {
        printf("%d: %d\n", i, pids[i]);
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
        // Exit child process before the parent process
        exit(1);
      }
      else // Parent process
      {
        int status;
        waitpid(pid, &status, 0);
        history_index = updateHistory(history, history_index, command_string);
        pids_index = updatePids(pids, pids_index, pid);
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
  if (history_index == MAX_HISTORY_SIZE)
  {  
    for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++)
    {
      strcpy(history[i], history[i+1]);
    }
    strcpy(history[MAX_HISTORY_SIZE-1], command_string);
  }
  else
  {
    strcpy(history[history_index], command_string);
    history_index++;
  }

  return history_index;
}

int updatePids(int pids[MAX_PID_SIZE], int pids_index, int pid)
{
  if (pids_index == MAX_PID_SIZE)
  {
    for (int i = 0; i < MAX_PID_SIZE - 1; i++)
    {
      pids[i] = pids[i+1];
    }
    pids[MAX_PID_SIZE-1] = pid;
  }
  else
  {
    pids[pids_index] = pid;
    pids_index++;
  }

  return pids_index++;
}
