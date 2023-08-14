#include "../headers/controls.hpp" 

#include <iostream>

/*
    Text box and built in PityBoy code editor

    Splited into another file because it's getting big

*/

namespace PityBoy::controls {

    textBox::textBox(int x, int y, int w, int h) { // width and height are here *8 !!
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;

        this->realW = w*8+3;
        this->realH = h*9+2;
        text = "WWWWWWWWWWWWWWWWWWWWW\nThis is line 2\ntstsetst\nbadgdshaeysa\nabc\nabc\nabc\nabc\ncrap\nmore crap\nidk to write";
        convertTextIntoVector();

        cursorPosX = 0;
        cursorPosY = 0;
        

        defaultCursor.loadFromSystem(sf::Cursor::Arrow);
        textCursor.loadFromSystem(sf::Cursor::Text);
        lastCursor = 0; // 0 arrow 1 text

        if((signed)textLines.size()==0) {
            textLines.push_back("");
        }
    }

    void textBox::draw(PityBoy::controls::Parent* myWindow) {
        frameCounter++;
        myWindow->drawBox(x, y, realW, realH, 2);
        myWindow->drawBox(x, y, realW, realH, 3, true);

        for(unsigned int i=0;i<(unsigned)h;i++) { // unsigned to prevent compiler warnings
            if(scrollY+i>=textLines.size()) break;
            std::string splitStr = textLines.at(scrollY+i);
            if(scrollX<(signed)splitStr.length()) { // when to long to substr then just dont draw it

                splitStr = splitStr.substr(scrollX, w);
                myWindow->drawText(x+2, y+2+i*9, splitStr, 1);
            }
        }

        sf::Vector2i mousePos = myWindow->getMousePos();

        // check if mouse is over text box to change pointer to text cursor
        if(isMouseOver(x, y, realW, realH, mousePos.x, mousePos.y)&&myWindow->isFocused()) { // mouse cursor
            if(!lastCursor) {
                myWindow->setMouseCursor(&textCursor);
            }
            lastCursor = true;
        } else {
            if(lastCursor) {
                myWindow->setMouseCursor(&defaultCursor);
            }
            lastCursor = false;
        }

        if(focused) { // in textbox blinking cursor
            if(frameCounter%60<30) {
                int dx=cursorPosX - scrollX;
                int dy=cursorPosY - scrollY;

                if(isTextCursorOnScreen()) { // check is the cursor in bounds on screen
                    for(int i=0; i<8; i++) {
                        myWindow->drawPixel(x+2+dx*8-1, y+2+dy*9+i, 0);
                    }
                    //myWindow->drawChar(x+2+dx*8, y+2+dy*9, '|', 0);
                }
            }
        }
    }

    void textBox::handleEvent(sf::Event event, PityBoy::controls::Parent* myWindow) {

        if(event.type == sf::Event::MouseButtonPressed) {

            if(event.mouseButton.button == sf::Mouse::Left) { // pressed on text box
            
                sf::Vector2i mousePos = myWindow->getMousePos();
                
                if(isMouseOver(x+1, y+1, realW-3, realH-3, mousePos.x, mousePos.y)) { // textbox: when mouse pressed on it
                    focused = true;
                    held = true;
                    frameCounter = 0;

                    // calculate block cursor position based from mouse coordinates on mouse press on text box
                    // note: i think it is better to don't touch this code
                    int cx=mousePos.x - x - 5;
                    int cy=mousePos.y - y - 1;
                    int cxfix = ceil(cx/(float)8); // fix for X going outside view area due to calculation
                    if(cxfix==w) cxfix--;
                    cursorPosX = scrollX +  cxfix;
                    cursorPosY = scrollY + ceil(cy/(float)9) - 1;
                    

                    if(cursorPosY >= (signed)textLines.size()) { // outside Y
                        cursorPosY = textLines.size() - 1;
                        cursorPosX = textLines.at(cursorPosY).length();
                    }

                    if(cursorPosX > (signed)textLines.at(cursorPosY).length()) {
                        cursorPosX = textLines.at(cursorPosY).length();
                    }

                    rememberXPos = cursorPosX;

                } else {
                    focused = false;
                }
            }

        }

        if(event.type == sf::Event::MouseButtonReleased) {

            if(event.mouseButton.button == sf::Mouse::Left) {

                sf::Vector2i mousePos = myWindow->getMousePos();
                if(isMouseOver(x, y, realW, realH, mousePos.x, mousePos.y) && held) {
                    // ...
                } 
                held = false;
            }

        }

        if (event.type == sf::Event::MouseWheelScrolled) { // textbox: wheel support
            
            sf::Vector2i mousePos = myWindow->getMousePos();

            if(isMouseOver(x, y, realW, realH, mousePos.x, mousePos.y)) {

                if(event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel && allowNewLines) { // Y
                    scrollY = scrollY - event.mouseWheelScroll.delta;
                    if(scrollY < 0) scrollY = 0;
                    int maxScrollY = (signed)textLines.size();
                    if(scrollY >= maxScrollY) scrollY = maxScrollY-1;
                }

                if(event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel) { // X (untested with mouse)
                    scrollX = scrollX - event.mouseWheelScroll.delta;
                    if(scrollX < 0) scrollX = 0;
                    int maxScrollX = getTextLongestX();
                    if(scrollX >= maxScrollX) scrollX = maxScrollX-1;
               }
                    
            }

        }   

        if(event.type == sf::Event::TextEntered) { // textbox: input

            bool canInsertChar=true;

            if(maxLength!=0) { // check length of our string
                int i=0;
                int end=textLines.size();
                int totalLength=0;
                for(const std::string& b : textLines ) {
                    i++;
                    totalLength += b.length();
                    if(i!=end) totalLength++; // include new lines 
                }
                if(totalLength>=maxLength) canInsertChar=false;
            }

            if(event.text.unicode > 31 && event.text.unicode < 128 && canInsertChar) { // any printable character
                
                std::string currentLine = textLines.at(cursorPosY);
                
                currentLine = currentLine.substr(0,cursorPosX) + char(event.text.unicode) + currentLine.substr(cursorPosX);
                textLines.at(cursorPosY) = currentLine;

                moveCursor(right);
            }

            if(event.text.unicode == 13 && canInsertChar) { // textbox: newline

                if(allowNewLines) { // if textbox is multiline

                    textLines.insert(textLines.begin() + cursorPosY + 1, "") ;

                    if(cursorPosX==(signed)textLines.at(cursorPosY).length()) { // if at the end of line, just do nothing
                        moveCursor(right);
                    } else { // if not, substring the line
                        std::string tempStr = textLines.at(cursorPosY).substr(cursorPosX);
                        textLines.at(cursorPosY) = textLines.at(cursorPosY).substr(0,cursorPosX);
                        moveCursor(right);
                        textLines.at(cursorPosY) = tempStr;
                    }
                }
            }


            if(event.text.unicode == 8) { // textbox: backspace
                std::string currentLine = textLines.at(cursorPosY);

                // if line length is 0, just remove the line
                if(currentLine.length()==0) {

                    if(cursorPosY!=0) { // we cannot remove at begining

                        int tempY = cursorPosY; 
                        moveCursor(left);
                        textLines.erase(textLines.begin()+tempY); // remove the line
                    }

                } else {
                    if(cursorPosX==0) { // if we are at beginning, remove the line but move the contents

                        int tempY = cursorPosY; // before moving shift the cursor
                        moveCursor(left);
                        std::string tempStr = textLines.at(tempY); // get the line
                        textLines.erase(textLines.begin()+tempY);  // remove the line
                        // connect the line to next line
                        textLines.at(cursorPosY) = textLines.at(cursorPosY) + tempStr;
                    } else {
                        currentLine = currentLine.substr(0,cursorPosX-1) + currentLine.substr(cursorPosX);
                        textLines.at(cursorPosY) = currentLine;
                        moveCursor(left);
                    }
                }
            }


        }


        if(event.type == sf::Event::KeyPressed) { // textbox: special keys support

            frameCounter = 0;

            // textbox: arrows
            if(event.key.code == sf::Keyboard::Right) { // right arrow
                moveCursor(right);    
            }
            if(event.key.code == sf::Keyboard::Left) { // Left arrow
                moveCursor(left);    
            }

            if(event.key.code == sf::Keyboard::Down) { // Down arrow
                moveCursor(down);       
            }
            if(event.key.code == sf::Keyboard::Up) { // Up arrow
                moveCursor(up); 
            }

            // textbox: home and end
            if(event.key.code == sf::Keyboard::Home) { // Home
                if(event.key.control) {
                    cursorPosX = 0;
                    cursorPosY = 0;
                } else {
                    cursorPosX = 0;    
                }
                scrollToCursor();
                rememberXPos = cursorPosX;
            }
            if(event.key.code == sf::Keyboard::End) { // End
                if(event.key.control) {
                    // to move properly viewport, lets just move cursor using for loop
                    cursorPosY = textLines.size() - 1;
                    cursorPosX = textLines.at(cursorPosY).length();
                    scrollToCursor();
                } else {
                    for(int i=cursorPosX; i < (signed)textLines.at(cursorPosY).length(); i++) {
                        moveCursor(right);
                    }
                }
                rememberXPos = cursorPosX;
            }

            // textbox: Page Down and Page Up
            if(event.key.code == sf::Keyboard::PageDown) {
                cursorPosY = cursorPosY + h + 1; // some extended code to fix viewport scroll
                if(cursorPosY > (signed)textLines.size()) cursorPosY = (signed)textLines.size();
                scrollToCursor();
                moveCursor(up);
            }

            if(event.key.code == sf::Keyboard::PageUp) {
                cursorPosY = cursorPosY - h + 1; // some extended code to fix viewport scroll
                if(cursorPosY < 1) cursorPosY = 1;
                moveCursor(up);
            }

        }

    } 

    /*
        Big TO DO LIST:
        ##  Working with Words:
        Ctrl + Left Arrow: Move cursor to the beginning of the previous word.
        Ctrl + Right Arrow: Move cursor to the beginning of the next word.
        Ctrl + Backspace: Delete the previous word.
        Ctrl + Delete: Delete the next word.
        Ctrl + Up Arrow: Move cursor to the beginning of the paragraph.
        Ctrl + Down Arrow: Move cursor to the end of the paragraph.
        ## Moving the Cursor:
        Home: Move cursor to the beginning of the current line.                 DONE
        End: Move cursor to the end of the current line.                        DONE
        Ctrl + Home: Move cursor to the top of the text entry field.            DONE
        Ctrl + End: Move cursor to the bottom of the text entry field.          DONE
        Page Up: Move cursor up a frame.                                        DONE
        Page Down: Move cursor down a frame.                                    DONE
        ## Selecting Text:
        Shift + Left/Right Arrow Keys: Select characters one at a time.
        Shift + Up/Down Arrow Keys: Select lines one at a time.
        Shift + Ctrl + Left/Right Arrow Keys: Select words. Keep pressing the arrow keys to select additional words.
        Shift + Ctrl + Up/Down Arrow Keys: Select paragraphs.
        Shift + Home: Select the text between the cursor and the beginning of the current line.
        Shift + End: Select the text between the cursor and the end of the current line.
        Shift + Ctrl + Home: Select the text between the cursor and the beginning of the text entry field.
        Shift + Ctrl + End: Select the text between the cursor and the end of the text entry field.
        Shift + Page Down: Select a frame of text below the cursor.
        Shift + Page Up: Select a frame of text above the cursor.
        Ctrl + A: Select all text.
        ## Editing:
        Ctrl + C: Copy selected text.
        Ctrl + X: Cut selected text.
        Ctrl + V: Paste text at cursor.
        Ctrl + Z: Undo.
        Ctrl + Y: Redo.
    */

    void textBox::convertTextIntoVector() { // as function says: convert "string text" into line split vector
        textLines.clear();
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line)) {
            std::replace( line.begin(), line.end(), '\t', ' '); 
            textLines.push_back(line);
        }
    }

    int textBox::getTextLongestX() {
        auto textLinesLongest = std::ranges::max_element(textLines, std::ranges::less{}, std::ranges::size);
        return textLinesLongest->length();
    }

    bool textBox::isTextCursorOnScreen() {
        int dx=cursorPosX - scrollX;
        int dy=cursorPosY - scrollY;

        if(dx>=0 && dy>=0 && dx<w && dy<h) return true;
        return false;
    }

    void textBox::scrollToCursor() { // this function force scrolls when cursor is outside
        if(!isTextCursorOnScreen()) {
            scrollX = cursorPosX-w+1;
            if(scrollX<0) scrollX = 0;

            // check if cursor is really invisible on Y axis before forcing scrollY

            if(!((cursorPosY-scrollY) >= 0 && (cursorPosY-scrollY) < h)) {
                scrollY = cursorPosY;
            }
        }
    }

    void textBox::moveCursor(moveDirection direction) { // the black magic happens here...

        if(direction == moveDirection::right) {
            cursorPosX++;
            if(cursorPosX>(signed)textLines.at(cursorPosY).length()) { // if at the end
                if(cursorPosY+1 < (signed)textLines.size()) { // next line if we can
                    cursorPosX = 0;
                    cursorPosY++;
                } else {
                    cursorPosX--;
                }  
            }
            // went out of bounds Y?
            if((cursorPosY - scrollY)>=h) {
                scrollY++;
            } 

            // if went out of bounds X, move the scrollX
            if((cursorPosX - scrollX) >= w) scrollX++;
            rememberXPos = cursorPosX;
        }
        
        if(direction == moveDirection::left) {
            cursorPosX--;
            if(cursorPosX<0) { // if at the beg
                if(cursorPosY-1>=0) { // go to end of line before
                    cursorPosY--;
                    cursorPosX = textLines.at(cursorPosY).length();
                } else {
                    cursorPosX=0;
                }
            }

            // went out of bounds?
            if((cursorPosX - scrollX) < 0) scrollX--;
            rememberXPos = cursorPosX;
        }

        if(direction == moveDirection::down) {
            cursorPosY++;

            if(cursorPosY>=(signed)textLines.size()) cursorPosY--;

            // rememberXPos logic
            cursorPosX = rememberXPos;

            if((signed)textLines.at(cursorPosY).length()<cursorPosX) { // if we are beyond line on X, go back to line
                cursorPosX = textLines.at(cursorPosY).length();
            }

            // went out of bounds Y?
            if((cursorPosY - scrollY)>=h) {
                scrollY++;
            } 
        }

        if(direction == moveDirection::up) {
            cursorPosY--;

            if(cursorPosY<0) cursorPosY=0;

            // rememberXPos logic
            cursorPosX = rememberXPos;

            if((signed)textLines.at(cursorPosY).length()<cursorPosX) { // if we are beyond line on X, go back to line
                cursorPosX = textLines.at(cursorPosY).length();
            }

            // went out of bounds Y?
            if((cursorPosY - scrollY)<0) {
                scrollY--;
            } 
        }

        scrollToCursor();
    }

    void textBox::setDimensions(int x,int y, int w,int h) {
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;
    }

    void textBox::setConfig(bool allowNewLines, int maxLength) {
        this->allowNewLines = allowNewLines;
        this->maxLength = maxLength;
    }

}