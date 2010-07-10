![cherami - the simple and secure mail transfer agent](http://x1598.at/cherami-logo.png)

Goals
======
* easy to setup and use
* reasonable default configuration
* security against malicious attacks

Easy Setup & Default Configuration
==================================
Configuration should be as simpel as possible. I'm thinking about 
configuring a few details (which domains to accept, authentication mode) and 
probably a list of valid real users and a list for virtual users (mailinglists for example).
The rest should work out of the box.

Security
=======

Parser
-------
The main part of cherami is the SMTP parser which was written using lex/yacc.
Parsers are probably the number 1 security issue so it's important that the SMTP
parser works in a well defined way.

Principle of least privileges
-----------------------------
The MTA is broken into a few parts to reassure that each part only has to have the necessary
privileges. In case of a break-in, the other part of the system should not be affected in any way.

Secure String Handling
----------------------
String operations have to be carefully checked to avoid buffer overflows of any kind. Each allocated 
String has to be freed.

Error Checking
--------------
Any Function that could return an error, has to be error-checked after calling it. This is necessary for
maximum transparency and ease of debugging.
