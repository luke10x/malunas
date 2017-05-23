Feature: Proxy incomming connections to further destination 

  Scenario: Http server responds through proxy 

    Given server started with '--workers=1 0 proxy web:10080' 

     When client connects
     When client sends 'GET / HTTP/1.0\r\n\r\n'
     Then client receives 'Hello, this is a web server'
