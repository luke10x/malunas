Feature: TTY ensures that program output immediatelly goes back to client
  When TTY is enabled
  Even when program does not flush
  server sends all programs output to client

  Scenario: client does not see response if program does not flush and TTY is not used
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects
      And program writes 'hello'
     Then client does not receive anything
      And program exits 

  Scenario: client sees response if program flushes and TTY is not used
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects
      And program handles the request
      And program writes 'hello'
      And program program flush output
     Then client receives 'hello' 
      And program exits 

  Scenario: client sees response if program does not flush and exits and TTY is not used
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects
      And program handles the request
      And program writes 'hello'
      And program exits 
     Then client receives 'hello' 

  Scenario: client sees response immediately if program does flushes and TTY is used
    Given number of workers is 1
      And TTY mode is on
      And verbose mode is off
      And server is started
     When client connects
      And program handles the request
      And program writes 'hello'
      And program program flush output
     Then client receives 'hello' 
      And program exits 
