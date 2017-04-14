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

  Scenario: client sees response if program flushes and TTY is not used
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects
      And program writes 'hello'
      And program program flush output
     Then client receives 'hello' 

  Scenario: client sees response if program does not flush and exits and TTY is not used
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects
      And program writes 'hello'
      And program program exits 
     Then client receives 'hello' 

  Scenario: client sees response immediately if program does not flush but TTY is used
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects
      And program writes 'hello'
     Then client receives 'hello' 
