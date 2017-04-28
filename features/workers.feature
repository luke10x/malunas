Feature: Several workers are accepting client connections 

  Scenario: one worker can only serve one client at a time

    Given server started with '--workers=1 0 exec python program.py' 

     When client connects  
      And second client connects
      And program handles the request
     Then program does not handle request

  Scenario: two workers can serve two clients at time  

    Given server started with '--workers=2 0 exec python program.py' 

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

