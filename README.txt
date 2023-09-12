USAGE:
1.) Open three terminals
2.) Compile with command "make"
3.) In first terminal, enter "./ttts 15000"
4.) In second and third terminal, enter "./ttts <domain> 15000"
5.) Follow message prompts to play tic-tac-toe

PROJECT DESCRIPTION AND ASSUMPTIONS:
This project is a simple on-line multiplayer tic-tac-toe. It is able to handle one game at a time. 
Players connect to the server, enter their name, and they are then assigned a role. From there, the players 
follow the message prompts to complete the game. We made some assumptions, mainly that the coordinates (x,y) 
compute in the same way you would traverse 2D array. For instance, the coordinates 3,2 would result in a mark
being placed in the third row, second column.

TEST PLAN:
The scenarios we wanted to ensure would be handled smoothly:

1.) Full board with no winner
    We have several checks in our code that checks everytime a move is made whether there is a winner. After those checks, we go through the entire 
    board, and if there are no empty spots in the board, then we can say that it is a draw. When we simulate a game that ends in a draw, our 
    code successfully displays OVER, signifying that the board is full with no winner.
2.) Draw request, accept, and suggest
    We also want to ensure that draws are handled smoothly. There are three things we need to check:
        a.) If someone suggests a draw, does the opponent receive that request
        b.) If someone accepts a draw, does the game terminate
        c.) If someone rejects a draw, does the person who suggested the draw then get to play their turn
    We tested all three scenarios. For a, we simply sent a draw request, which our code handled perfectly. For b, we tested whether accepting a draw 
    would end the game, which it did. For c, we rejected a draw, and confirmed that the person who suggested the draw received the message that their
    draw was rejected, as well as that they could then make their actual move
3.) Invalid messages
    Our code handles invalid messages well. We first tested if invalid coordinates were handled correctly, which they were, invalid coordinates are 
    rejected. Secondly, if a player inputs a nonsense message, or a command such as MOVD, the code rejects the message and prompts them to enter a valid
    command. This is because our code checks if the client sends any of the listed commands in the assignment page, and if not, then we automatically 
    assume that the message is nonsense.
4.) Ctrl C 
    We tested how our code handles Ctrl C. The client automatically terminates the program with Ctrl C. The server usually requires Ctrl C to be entered 
    twice, which we are not sure as to why. 
5.) Name input
    We have a character limit of 64 characters, along with the requirement that the names of two players cannot be the same. We first checked how the code
    handles a name that is too long. When we try that, the code rejects the name, prompting the client to re-enter a valid name. If the name is the
    same as the opponent, the code rejects the name, prompting the client to enter a unique name.
6.) General win/Resignation
    We tested all cases of a win, on both sides. This means we checked whether the code can detect diagonal wins, horizontal wins, and vertical wins,
    for both player sides. We also checked to ensure that resigning properly indicates who wins and who loses. 
7.) Server
    We sometimes have issue when running the server. After terminating the code, when we try to rerun the server, we 
    will get a bind error. This can be resolved usually by just waiting a little bit, or by restarting the terminal. We were not sure how to resolve this
    issue through code.

DESIGN:
We wanted to make the code user-friendly. Therefore, when the client connects to a server, the client then just has to enter their name. The client 
code automatically appends their name to a PLAY message. For things like making a move, the client simply has to enter their coordinates, and 
again the client sends the proper MOVE message. To resign, the client enters q, to draw, d, and to accept a draw, the client enters y, and to reject a 
draw the client enters n. Again, we wanted to make this as easy to use for the client as possible. If we were to return to this project, we would
work on printing an actual tic-tac-toe board each move, further enhancing client usability. Our ttts code is able to work with other clients as well, as 
the messages it receives and sends are formatted to the specifications of the assignment. ttts also has some helpful print statements.
For more in depth design notes, please refer to the comments located in our code. 

MISC:
We tried to implement multi-threading so that our code could handle multiple games at the same time. However, we did run into the difficulty of the 
multi-threading not multi-threading. We have included some of our modified code in the file "mtc.txt", to show our attempts. We used pthread, and 
consolidated all of the game info into a game struct, which we held in a games array, so that we could keep track of all the games being run, removing
and adding from as necessary. 

Thank you for reading!
