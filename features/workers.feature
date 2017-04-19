Feature: Several workers are accepting client connections 

  Scenario: one worker can only serve one client at a time

    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started

     When client connects  
      And second client connects
      And program handles the request
     Then program does not handle request

  Scenario: two workers can serve two clients at time  

    Given number of workers is 2
      And TTY mode is off
      And verbose mode is off
      And server is started

     When client connects  
      And second client connects
      And program handles the request
      And second program handles the request
     Then program does not handle request

     When program writes 'hello'
     When program flushes output
     Then client receives 'hello' 

     When second program writes 'hello'
     When second program flushes
     Then second client receives 'hello' 

