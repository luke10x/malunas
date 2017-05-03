Feature: request handling program can be run in a TTY  
  TTY is not buffered, therefore it does not even require flush

  Scenario: With TTY client sees response immediately even if program does not flush

    Given server started with '--workers=1 --tty 0 exec python scripts/program.py' 

     When client connects
      And program handles the request
     Then program recognizes that it runs in TTY

     When program writes 'hello'
     Then client receives 'hello' 

  Scenario: Without TTY client does not see any response if program does not flush

    Given server started with '--workers=1 0 exec python scripts/program.py' 

     When client connects
      And program handles the request
     Then program recognizes that it does not run in TTY

     When program writes 'hello'
     Then client does not receive anything

     When program exits 
     Then client receives 'hello'

  Scenario: Without TTY client sees response immediately if program flushes

    Given server started with '--workers=1 0 exec python scripts/program.py' 

     When client connects
      And program handles the request
     Then program recognizes that it does not run in TTY

     When program writes 'hello'
      And program flushes output
     Then client receives 'hello'
