//******************************************************************************
// Checkers.java:	Applet
// (C)opyright 1997 Victor Huang and Sung Ha Huh. All rights reserved.
//******************************************************************************
import java.applet.*;
import java.awt.*;
import java.util.Date;

//==============================================================================
// Main Class for applet Checkers
//
//==============================================================================
public class Checkers extends Applet
{
	int[][] position = new int[8][8];
	
	static final int WHITE = 1;
	static final int BLACK = 2;
	static final int WKING = 3;
	static final int BKING = 4;
	static final int EMPTY = 0;
	int toMove = BLACK;
	int loser = EMPTY;
	Choice whitePlayer = new Choice();
	Choice blackPlayer = new Choice();
	Label toMoveLabel = new Label();
	Choice maxDepth = new Choice();
	boolean running;
		
	Board board = new Board(this);
	Button newgameButton,passButton;

	// Checkers Class Constructor
	//--------------------------------------------------------------------------
	public Checkers()
	{
		System.out.println();
                System.out.println("Checkers Applet v1.02");
		System.out.println("Copyright 1997 by Victor Huang and Sung Ha Huh.");
		System.out.println("All rights reserved. Unauthorized reproduction prohibited.");
		System.out.println();
		setBackground(Color.white);
		whitePlayer.addItem("Human");
		whitePlayer.addItem("Computer");
		whitePlayer.select(1);
		blackPlayer.addItem("Human");
		blackPlayer.addItem("Computer");
		

		newgameButton = new Button("New game");
		passButton = new Button("Pass turn");

		for (int i=2;i<=6;i+=2)
		{
			maxDepth.addItem(Integer.toString(i));
		}

		maxDepth.select(1);

		GridBagLayout gridbag = new GridBagLayout();
		GridBagConstraints c = new GridBagConstraints();
		setLayout(gridbag);
		
		c.fill = GridBagConstraints.NONE;
		c.anchor = GridBagConstraints.CENTER;
		c.gridwidth = 1;
		c.gridheight = 1;
		c.gridx = 0;
		c.gridy = 0;
		Label whiteLabel = new Label("White ");
		whiteLabel.setAlignment(Label.RIGHT);
		gridbag.setConstraints(whiteLabel,c);
		add(whiteLabel);

		c.gridx = 1;
		gridbag.setConstraints(whitePlayer,c);
		add(whitePlayer);

		c.gridx = 2;
		Label blackLabel = new Label("Black ");
		blackLabel.setAlignment(Label.RIGHT);
		gridbag.setConstraints(blackLabel,c);
		add(blackLabel);

		c.gridx = 3;
		gridbag.setConstraints(blackPlayer,c);
		add(blackPlayer);

		c.gridx = 0;
		c.gridy = 1;
		c.gridheight = 8;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.insets = new Insets(5,5,5,5);
		this.board.resize(240,240);
		gridbag.setConstraints(this.board,c);
		add(this.board);

		c.insets = new Insets(0,0,0,0);
		c.gridy = 9;
		c.gridheight = 1;
		c.gridwidth = 2;
		c.gridx = 1;
		c.fill = GridBagConstraints.BOTH;
		this.toMoveLabel.setAlignment(Label.CENTER);
		gridbag.setConstraints(this.toMoveLabel,c);
		add(this.toMoveLabel);

		c.fill = GridBagConstraints.NONE;
		c.gridx = 0;
		c.gridy = 10;
		Label sdLabel = new Label("Search Depth");
		gridbag.setConstraints(sdLabel,c);
		add(sdLabel);

		c.gridx = 2;
		gridbag.setConstraints(maxDepth,c);
		add(maxDepth);

		c.fill = GridBagConstraints.BOTH;
		c.gridx = 0;
		c.gridy = 11;
		gridbag.setConstraints(newgameButton,c);
		add(newgameButton);

		c.gridx = 2;
		gridbag.setConstraints(passButton,c);
		add(passButton);

		c.gridx = 0;
		c.gridy = 12;
		c.gridwidth = GridBagConstraints.REMAINDER;
		Label adLabel = new Label("Created by Victor Huang and Sung Ha Huh");
		adLabel.setForeground(Color.blue);
		adLabel.setBackground(Color.lightGray);
		adLabel.setAlignment(Label.CENTER);
		gridbag.setConstraints(adLabel,c);
		add(adLabel);

	}

	// APPLET INFO SUPPORT:
	//		The getAppletInfo() method returns a string describing the applet's
	// author, copyright date, or miscellaneous information.
    //--------------------------------------------------------------------------
	public String getAppletInfo()
	{
		return "Name: Checkers\r\n" +
		       "Authors: Victor Huang and Sung Ha Huh\r\n" +
			   "Written May 1997 for Caltech's CS20c class\r\n" +
                           "http://www.cs.caltech.edu/~vhuang/cs20/c/\r\n" +
			   "CS20's URL: http://www.cs.caltech.edu/~cs20/\r\n";
	}


	// The init() method is called by the AWT when an applet is first loaded or
	// reloaded.  Override this method to perform whatever initialization your
	// applet needs, such as initializing data structures, loading images or
	// fonts, creating frame windows, setting the layout manager, or adding UI
	// components.
    //--------------------------------------------------------------------------
	public void init()
	{
        resetBoard();
		// TODO: Place additional initialization code here
	}

	// Place additional applet clean up code here.  destroy() is called when
	// when you applet is terminating and being unloaded.
	//-------------------------------------------------------------------------
	public void destroy()
	{
		// TODO: Place applet cleanup code here
	}

	public void start()
	{
		running = true;
	}

	public void stop()
	{
		running = false;
	}

	// Checkers Paint Handler
	//--------------------------------------------------------------------------
	public void paint(Graphics g)
	{
	}

	private void resetBoard()
	{
		this.board.highlight = false;
		this.board.incomplete = false;
		loser = EMPTY;
		
		// set up checkers
		for (int i=0; i<8; i++)
		{
			for (int j=0; j<8; j++)
				position[i][j] = EMPTY;

			for (int j=0; j<3; j++)
			if ( possible_square(i,j) )
				position[i][j] = WHITE;	
		
			for (int j=5; j<8; j++)
			if ( possible_square(i,j) )
				position[i][j] = BLACK;
		}
		this.toMove = BLACK;
		updateMoveLabel();
		if (blackPlayer.getSelectedIndex() == 1)
		{
			this.toMove = WHITE;
			switch_toMove();
		}
		displayBoard();
	}

	void updateMoveLabel()
	{
		this.toMoveLabel.setForeground(Color.red);
		if (this.toMove == WHITE)
		{
			this.toMoveLabel.setBackground(Color.white);
			this.toMoveLabel.setText("White to move");
		}
		else 
		{
			this.toMoveLabel.setBackground(Color.black);
			this.toMoveLabel.setText("Black to move");
		}
		
		if (loser == WHITE)
		{
			this.toMoveLabel.setBackground(Color.black);
			this.toMoveLabel.setText("Black wins!");
		}
		else if (loser == BLACK)
		{
			this.toMoveLabel.setBackground(Color.white);
			this.toMoveLabel.setText("White wins!");
		}

	}

	public boolean action(Event evt, Object obj)
	{				  
		if (evt.target == newgameButton)
		{
			resetBoard();
			return true;
		}

		else if (evt.target == passButton)
		{
			switch_toMove();
			this.board.highlight = false;
			this.board.incomplete = false;
			displayBoard();
			return true;
		}

		else if (evt.target == whitePlayer || evt.target == blackPlayer)
		{
			this.board.highlight = false;
			this.board.incomplete = false;
			displayBoard();
			return true;
		}

		else if (evt.target == maxDepth)
			return true;

		
			
		
		//return super.handleEvent(evt);
		return false;
	} // action()

	public boolean handleEvent(Event evt)
	{

		if (evt.target == whitePlayer && whitePlayer.getSelectedIndex() == 1
			&& this.toMove == WHITE)
		{
			this.toMove = BLACK;
			switch_toMove();
		}

		else if (evt.target == blackPlayer && blackPlayer.getSelectedIndex() == 1
			&& this.toMove == BLACK)
		{
			this.toMove = WHITE;
			switch_toMove();
		}
	
	
		return super.handleEvent(evt);
	}

	private void displayBoard()
	{
		this.board.paint(this.board.getGraphics());
		Thread.yield();
		
	}								   

	public void switch_toMove()
	{
		int score;
		int[] result = new int[4];
		int[] cMove;
		int[] counter = new int[1];
		Date date;

		counter[0]=0;
			
		// black just made his move
		displayBoard();
		if (this.toMove == BLACK && whitePlayer.getSelectedIndex()==1)
		{
			this.toMove = WHITE; 
			updateMoveLabel();
			date = new Date();
			long time = date.getTime();
			score = Engine.MiniMax(position,0,Integer.parseInt(maxDepth.getSelectedItem()),result,this.toMove,counter);
			date = new Date();
			time = date.getTime() - time;
            
			print_status(score,counter[0],time);


			if (result[0] == 0 && result[1] == 0)
				loser = WHITE;
			else
			{
				Move.move_board(position, result);
				if (running && loser == EMPTY && blackPlayer.getSelectedIndex()==1)
					switch_toMove();
				this.toMove = BLACK;
			}
		}

		// white just made his move
		else if (this.toMove == WHITE && blackPlayer.getSelectedIndex()==1)
		{
			this.toMove = BLACK; 
			updateMoveLabel();
			date = new Date();
			long time = date.getTime();
			score = Engine.MiniMax(position,0,Integer.parseInt(maxDepth.getSelectedItem()),result,this.toMove,counter);
			date = new Date();
			time = date.getTime() - time;
            print_status(score,counter[0],time);

			if (result[0] == 0 && result[1] == 0)
				loser = BLACK;
			else
			{
				Move.move_board(position, result);
				if (running && loser == EMPTY && whitePlayer.getSelectedIndex()==1)
					switch_toMove();
				this.toMove = WHITE;

			}
			
		}
		else 
		{
			if (this.toMove == WHITE)
				this.toMove = BLACK;
			else
				this.toMove = WHITE;
		}
		
		if (Move.noMovesLeft(position,this.toMove))
		{
			if (this.toMove == WHITE)
				loser = WHITE;
			else
				loser = BLACK;
		}
		updateMoveLabel();
		displayBoard();
	}

	public void update(Graphics g)
	{
		paint(g);
	} // update()

	boolean possible_square(int i, int j)
	{
		return (i+j)%2 == 1;
	}

	void print_status(int score, int positions, long time)
	{
		System.out.print("Score: ");
		System.out.print(score);
		System.out.print("\tPos : ");
		System.out.print(positions);
		System.out.print("\tTime : ");
		System.out.print(time);
		System.out.print("\tSpeed: ");
		if (time == 0) System.out.print(0);
		else System.out.print(positions*1000/time);
		System.out.println(" pos/sec");
	}
			
}
