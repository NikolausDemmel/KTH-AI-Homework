import java.awt.*;
import java.util.*;

/*
 *
 * Board
 *
 */
class Board extends Canvas
{
	boolean incomplete = false;
	boolean highlight = false;
	private int start_i,start_j,end_i,end_j;
	Checkers game;

	Image offImage;
	Graphics offGraphics;


	Board (Checkers checkers)
	{
		game = checkers;
	}

	public boolean mouseDown(Event evt, int x, int y)
	{
		
		int i = x / 30;
		int j = y / 30;

		if (game.toMove == Checkers.WHITE &&
			(game.position[i][j] == Checkers.WHITE ||
			 game.position[i][j] == Checkers.WKING)
			 ||
			 game.toMove == Checkers.BLACK &&
			 (game.position[i][j] == Checkers.BLACK ||
			 game.position[i][j] == Checkers.BKING))
		{

			// we don't want to lose the incomplete move info:
			// only set new start variables if !incomplete
			if (!incomplete)
			{
				highlight = true;
				start_i = i;
				start_j = j;
				repaint();
			}
		}
		else if ( highlight  && (float)(i+j) / 2 != (i+j) / 2)
		{
			end_i = i;
			end_j = j;
			int status = Move.ApplyMove(game.position,start_i,start_j,end_i,end_j);
			switch (status)
			{
			case Move.LEGALMOVE:
				incomplete = false;
				highlight = false;
				game.switch_toMove();
				break;
			case Move.INCOMPLETEMOVE:
				incomplete = true;
				highlight = true;
				// the ending square is now starting square
				// for the next capture
				start_i = i;
				start_j = j;
				break;
			} // switch
			repaint();
		} // else if
		return true;
	} // MouseDown()

	// to avoid flicker, the board is drawn to offImage using offscreen_paint
	public void paint(Graphics g)
	{
		if (offImage == null)
		{
			offImage = createImage(240,240);
			offGraphics = offImage.getGraphics();
		}
		offscreen_paint(offGraphics);
		g.drawImage(offImage,0,0,this);
	}


	public void offscreen_paint(Graphics g)
	{
		
		// draw the checkerboard pattern
		for (int i=0; i<8; i++)
			for (int j=0; j<8; j++)
		{
			Color darkgreen = new Color(0,128,0);
			if ( (float)(i+j) / 2 != (i+j) / 2)
				g.setColor(darkgreen);	
			else
				g.setColor(Color.darkGray);
			g.fillRect(i*30,j*30,30,30);
			
			if (highlight && i==start_i && j==start_j)
			{
				if (!Move.canCapture(game.position,i,j) && Move.canCapture(game.position,game.toMove))
					g.setColor(Color.blue);
				else 
				g.setColor(Color.orange);
				g.fillRect(start_i*30,start_j*30,30,30);
			}

			switch (game.position[i][j])
			{
				case Checkers.WHITE:
				g.setColor(Color.white);
				g.fillOval(i*30+5,j*30+5,20,20);
				break;
				case Checkers.BLACK:
				g.setColor(Color.black);
				g.fillOval(i*30+5,j*30+5,20,20);
				break;
				case Checkers.WKING:
				g.setColor(Color.white);
				g.fillOval(i*30+5,j*30+5,20,20);
				g.setColor(Color.red);
				g.fillOval(i*30+10,j*30+10,10,10);
				break;
				case Checkers.BKING:
				g.setColor(Color.black);
				g.fillOval(i*30+5,j*30+5,20,20);
				g.setColor(Color.yellow);
				g.fillOval(i*30+10,j*30+10,10,10);
				
			} // switch
		} // for
		
	} // paint()


	// overriden so as to avoid flicker
	public void update(Graphics g)
	{
		paint(g);
	} // update()

} // Board

