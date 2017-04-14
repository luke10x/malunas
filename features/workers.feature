Feature: TTY ensures that program output immediatelly goes back to client
  When TTY is enabled
  Even when program does not flush
  server sends all programs output to client

  Scenario: one worker can only serve one client at a time
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client 1 connects  
     Then client 2 cannot connect 

  Scenario: two workers can serve two clients at time  
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client 1 connects  
      And client 2 connects
      And program 1 outputs 'hello1'
      And program 2 outputs 'hello2'
     Then client 1 receives 'hello1' 
      And client 2 receives 'hello2' 
      And client 3 cannot connect 

  Scenario: One worker can serve 2 clients one after another  
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client 1 connects  
      And program 1 exits
      And client 2 connects
      And program 1 outputs 'hello'
     Then client 2 receives 'hello' 
     
  Scenario: When client closes connection program is killed  
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects  
      And client disconnects
     Then program is not running 

  Scenario: When program exits client is disconnected  
    Given number of workers is 1
      And TTY mode is off
      And verbose mode is off
      And server is started
     When client connects  
      And program exits 
     Then client is disconnected 

