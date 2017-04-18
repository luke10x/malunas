Feature: request handling program can be run in a TTY  
  Normally output of a program running in the TTY, is automatically flushed,
  but that is a responsibility of a program anyway.

  Scenario: With TTY client does not see any response if program does not flush
    Given number of workers is 1
      And TTY mode is on
      And verbose mode is off
      And server is started

     When client connects
      And program handles the request
      And program writes 'hello'
     Then client does not receive anything

     When program exits 
     Then client receives 'hello' 

  Scenario: Without TTY client does not see any response if program does not flush
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started

     When client connects
      And program handles the request
      And program writes 'hello'
     Then client does not receive anything

     When program exits 
     Then client receives 'hello'

  Scenario: Without TTY client sees response immediately if program flushes
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started

     When client connects
      And program handles the request
      And program writes 'hello'
      And program flushes output
     Then client receives 'hello'
