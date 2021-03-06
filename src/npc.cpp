#include "npc.h"

NPC::NPC(string filename) {
    cout << "Constructing NPC\n";

    this->endConversation = false;

    this->filename = filename;
    this->startTopic = 'open';
    this->startPart = 0;
    this->currentText = "";
    this->paused = false;
    this->pauseCounter = 0;
    this->font = DEFAULT_FONT;
    this->textColor = sf::Color::White;
    this->fontHeight = 12;

    this->letterX = 0;
    this->letterY = 0;

    this->textSurface.create(1024, 100);

    this->dialogBox = nullptr;

    this->load();

    cout << "Done constructing NPC\n";
}

NPC::~NPC() {
    for (pair<string, NPCAnim*> p : this->characters) {
        delete p.second;
    }

}

void NPC::load() {
    cout << "Loading NPC\n";

    ifstream textFile;
    textFile.open("Data\\NPCs\\" + this->filename + "\\" + this->filename + ".txt");
    string line;
    vector<string> splitLine;  // Don't know if we need this yet
    bool loadingConversation = false;
    bool loadingAnimation = false;
    string animName = "";
    string characterName = "";
    Animation* animation;  // We might have to change this to a pointer.  I'm not sure yet.
    sf::Vector2i frameDimensions;
    sf::Vector2i frameCoord;
    sf::Rect<int> frameRect;

    this->textSurface.clear(sf::Color::Transparent);

    while (getline(textFile, line)) {

        // Conversation loading shenanigans
        if (startsWith(line, "!setStartTopic:")) {
            splitLine = splitString(line, ':');
            this->startTopic = splitLine[1];
        }
        if (startsWith(line, "!setStartPart:")) {
            splitLine = splitString(line, ':');
            this->startPart = stoi(splitLine[1]);
        }
        if (startsWith(line, "!addCharacter:")) {
            splitLine = splitString(line, ':');
            //sf::Texture spritesheet = imgLoad("Data\\NPCs\\" + this->filename + "\\" + splitLine[2]);
            NPCAnim* newCharacter = new NPCAnim("Data\\NPCs\\" + this->filename + "\\" + splitLine[2]);
            this->characters.insert({{splitLine[1], newCharacter}});
            // I really have no idea if this is going to work, but text seemed to work fine
        }
        if (startsWith(line, "!addImage:")) {
            splitLine = splitString(line, ':');
            string imgName = splitLine[1];
            this->images.insert({{imgName, imgLoad("Data\\NPCs\\" + this->filename + "\\" + splitLine[2])}});
        }

        if (startsWith(line, "LOAD_ANIMATION")) {
            splitLine = splitString(line, ':');
            loadingAnimation = true;
            characterName = splitLine[1];
            animName = splitLine[2];
            animation = new Animation();
            frameDimensions = sf::Vector2<int>(stoi(splitLine[3]), stoi(splitLine[4]));
        }
        if (loadingAnimation) {
            if (startsWith(line, "LOAD_ANIMATION")) {
                continue; // Do nothing
            } else if (startsWith(line, "END_ANIMATION")) {
                cout << "Done loading animation\n";
                cout << characterName << '\n';
                //this->animations.insert({{animName, animation}});
                this->characters[characterName]->addAnimation(animName, animation);
                loadingAnimation = false;
            } else {
                // How are we going to do this?  time:x:y
                splitLine = splitString(line, ':');
                cout << "splitLine[0]: " << splitLine[0] << '\n';
                double frameTime = stod(splitLine[0]);
                cout << "frameTime: " << frameTime << '\n';
                frameCoord = sf::Vector2<int>(stoi(splitLine[1]), stoi(splitLine[2]));
                frameRect = sf::Rect<int>(frameCoord.x * frameDimensions.x, frameCoord.y * frameDimensions.y, frameDimensions.x, frameDimensions.y);
                animation->addFrame(frameTime, frameRect);  // Let's see how this works
            }
        }

        if (startsWith(line, "LOAD_CONVERSATION")) {
            loadingConversation = true;
        }
        if (loadingConversation) {
            if (startsWith(line, "LOAD_CONVERSATION")) {
                continue;  // Do nothing
            } else if (startsWith(line, "END_CONVERSATION")) {
                loadingConversation = false;
            } else {
                // Load a topic into the conversation
                splitLine = splitString(line, '~');  // Split on the ~ first to get the topic name
                string newTopic = splitLine[0];
                vector<string> splitText = splitString(splitLine[1], '|');  // Now split on the |
                //splitLine.erase(splitLine.begin());  // Remove the topic marker so it doesn't show up in the conversation text
                this->text.insert({{newTopic, splitText}});
            }
        }
        // Done with loading conversations
        // We need stuff to load graphics, animations
    }
    this->destination = "quit:";

    cout << "Done loading NPC\n";
}

void NPC::render(sf::RenderWindow* window) {

    //window->draw(this->bkgSprite);
    window->draw(this->currentImage);

    for (pair<string, NPCAnim*> p : this->characters) {
        //p.second.load();
        p.second->tick();
        window->draw(p.second->sprite);
    }

    this->textSprite = sf::Sprite(this->textSurface.getTexture());
    this->textSprite.setPosition(0, 0);
    window->draw(this->textSprite);

    if (this->dialogBox != nullptr) {
        this->dialogBox->render(window, this);
    }

}

string NPC::play(sf::RenderWindow* window) {
    cout << "Calling NPC::play()\n\n";

    /*
    // Display the text map in its entirety
    for (pair<string, vector<string>> p : this->text) {
        cout << p.first << '\n';
        for (string s : p.second) {
            cout << '\t' << s << '\n';
        }
    }
    */

    this->resetAnimationStuff();

    this->topic = this->startTopic;
    this->part = this->startPart;
    this->currentText = this->text[this->topic][this->part];
    this->currentSplitText = splitString(this->currentText, ' ');

    this->currentWordIndex = 0;
    this->currentWord = this->currentSplitText[this->currentWordIndex];
    this->currentLetterIndex = 0;
    this->currentLetter = this->currentWord[this->currentLetterIndex];

    this->letterSurface = sf::Text(" ", DEFAULT_FONT, fontHeight);
    this->letterSurface.setColor(textColor);
    this->wordSurface = sf::Text(" ", DEFAULT_FONT, fontHeight);


    this->spaceLength = this->letterSurface.getLocalBounds().width;
    this->letterX = this->spaceLength;

    this->checkForFlags();

    this->clearSurface();
    this->xLimit = 1024;
    this->yLimit = 576;

    bool clicked;

    cout << "Done with loop preparations\n";
    while (window->isOpen()) {
        //cout << "In main loop\n";
        clicked = false;
        window->clear();

        this->mousePos = sf::Mouse::getPosition(*window);
        if (this->dialogBox != nullptr) {
            this->dialogBox->setMousePos(this->mousePos);
        }

        sf::Event event;
        while (window->pollEvent(event)) {
            if (this->dialogBox != nullptr) {
                this->dialogBox->takeInput(event, this);
            }
            if (event.type == sf::Event::Closed) {
                window->close();
            } else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    clicked = true;
                }
            }
        }

        if (this->endConversation) {
            return this->destination;
        }

        if (clicked) {
            if (this->dialogBox == nullptr) {
                // We need some way to quickly loop through the rest of the text so we don't miss flags.
                this->advance();
            } else {
                if (this->dialogBox->done) {
                    string topic = this->choices[this->dialogBox->getSubFocus()];
                    this->goTo(topic, 1);
                    delete this->dialogBox;
                    this->dialogBox = nullptr;
                }
            }
        }

        this->tick();
        this->render(window);
        window->display();
    }

    return "quit:";
}

void NPC::advance() {
    this->part += 1;
    this->currentText = this->text[this->topic][this->part];
    this->currentSplitText = splitString(this->currentText, ' ');
    this->resetAnimationStuff();

    this->checkForFlags();

    if (!this->stopAnimating) {  // I forget why these if statements are here...
        this->clearSurface();
    }
}

void NPC::advanceWord() {
    if (!this->stopAnimating) {
        if (this->currentWordIndex >= currentSplitText.size() - 1) {  // If we're on the last word
            this->stopAnimating = true;
            //this->advance();
        } else {  // Otherwise
            this->currentWordIndex++;
            this->currentLetterIndex = 0;
            this->currentWord = this->currentSplitText[this->currentWordIndex];
            this->currentLetter = this->currentWord[this->currentLetterIndex];
        }
    }
}

void NPC::resetAnimationStuff() {
    this->stopAnimating = false;
    this->currentSplitText = splitString(this->currentText, ' ');
    this->currentWordIndex = 0;
    this->currentWord = this->currentSplitText[this->currentWordIndex];
    this->currentLetterIndex = 0;
    this->currentLetter = this->currentWord[this->currentLetterIndex];
    this->letterX = this->spaceLength;
    this->letterY = 0;
}

void NPC::checkForFlags() {
    while (startsWith(this->currentWord, "!") && !this->stopAnimating && !this->paused) {
        // Do something, Taipu
        if (startsWith(this->currentWord, "!XX:")) {  // Exit
            this->endConversation = true;
            this->advanceWord();
        } else if (startsWith(this->currentWord, "!GT")) {  // Goto
            vector<string> splitFlag = splitString(this->currentWord, ':');
            this->goTo(splitFlag[1], stoi(splitFlag[2]));
        } else if (startsWith(this->currentWord, "!CA")) {  // Change animation
            vector<string> splitFlag = splitString(this->currentWord, ':');
            this->characters[splitFlag[1]]->changeAnimation(splitFlag[2]);
            this->advanceWord();
        } else if (startsWith(this->currentWord, "!CI")) {  // Change image
            cout << "Changing image\n";
            vector<string> splitFlag = splitString(this->currentWord, ':');
            this->currentImage.setTexture(this->images[splitFlag[1]]);
            // Center the image
            sf::FloatRect dimensions = this->currentImage.getLocalBounds();
            this->currentImage.setPosition((WX - (int)dimensions.width) / 2, (WY - (int)dimensions.height) / 2);
            this->advanceWord();
        } else if (startsWith(this->currentWord, "!PS:")) {  // Pause
            this->paused = true;
            this->pauseCounter = 0;
            this->pauseDuration = stod(splitString(this->currentWord, ':')[1]);
            this->advanceWord();
        } else if (startsWith(this->currentWord, "!SK:")) {  // Skip
            this->advance();
        } else if (startsWith(this->currentWord, "!CB:")) {  // Choice box
            cout << "Created choice box\n";
            vector<string> splitFlag = splitString(this->currentWord, ':');
            vector<string> choiceTitles;
            vector<bool> usables;
            this->choices.clear();
            for (int i=1; i<splitFlag.size(); i++) {
                this->choices.push_back(splitFlag[i]);
                choiceTitles.push_back(this->text[splitFlag[i]][0]);
                usables.push_back(true);
            }
            this->dialogBox = new ChoiceInputBox(sf::Vector2<int>(0, 200), choiceTitles, usables, choiceTitles.size());  // Fill this in
            this->stopAnimating = true;
            break;
        } else if (startsWith(this->currentWord, "!CD:")) {  // Change destination
            vector<string> splitFlag = splitString(this->currentWord);
            this->destination = splitFlag[1] + ":" + splitFlag[2];
        } else {  // A flag we haven't implemented yet
            cout << "Unimplemented flag: " << this->currentWord << '\n';
            this->advanceWord();
        }
    }
}

void NPC::clearSurface() {
    this->textSurface.clear(sf::Color::Transparent);  // We might have to get more elaborate later but I think this works
}

void NPC::goTo(string topic, int part) {
    this->topic = topic;
    this->part = part;
    this->currentText = this->text[this->topic][this->part];
    this->currentSplitText = splitString(this->currentText, ' ');
    this->resetAnimationStuff();
    this->checkForFlags();

    if (!this->stopAnimating) {
        this->clearSurface();
    }
}

void NPC::resetConversation() {
    this->goTo(this->startTopic, this->startPart);
}

void NPC::tick() {
    // Do something, Taipu
    // Blit 1 letter per call.  For now.  Can be changed later to a delta-time system.
    /*!CI:talk
    cout << "Calling tick()\n";
    cout << "topic: " << this->topic << '\n';
    cout << "part: " << this->part << '\n';
    cout << "currentText: " << this->currentText << '\n';
    cout << "currentWord: " << this->currentWordIndex << ", " << this->currentWord << '\n';
    cout << "currentLetter: " << this->currentLetterIndex << ", " << this->currentLetter << '\n';
    */

    if (!this->stopAnimating) {
        if (!this->paused) {
            // Draw a letter
            // How the doomsday do we transfer this particular bit of Python to SFML?
            //cout << "Drawing letter\n";

            //this->letterSurface = sf::Text("", DEFAULT_FONT, 20);
            this->letterSurface.setString(this->currentLetter);
            //this->letterSurface.setColor(sf::Color::White);
            sf::FloatRect r = this->letterSurface.getLocalBounds();
            this->letterSize = sf::Vector2<int>(r.width, r.height);
            this->letterSurface.setPosition(this->letterX, this->letterY);
            //this->letterSurface.setPosition(0,0);
            this->textSurface.draw(this->letterSurface);
            //window->draw(this->letterSurface);
            this->letterX += this->letterSize.x;
            this->currentLetterIndex += 1;

            //cout << "Letter drawn: " << this->currentLetter << ", (" << this->letterX << ", " << this->letterY << ")\n";

            if (this->currentLetterIndex >= this->currentWord.size()) {  // If we're on the last letter
                if (this->currentWordIndex >= currentSplitText.size() - 1) {  // If we're on the last word
                    this->stopAnimating = true;
                    return;
                } else {  // If we're not on the last word
                    // Go to the next word
                    //cout << "Going to next word\n";
                    if (currentLetter == '.' || currentLetter == '!' || currentLetter == '?') {  // If it ends with a punctuation mark
                        this->letterX += (this->spaceLength * 2);  // Add double space
                    } else {
                        this->letterX += this->spaceLength;  // Add single space
                    }

                    this->currentLetterIndex = 0;
                    this->currentWordIndex += 1;
                    this->currentWord = this->currentSplitText[this->currentWordIndex];  // Problem line
                    //cout << "Changing current word\n";!CI:talk

                    while (this->currentWord.size() == 0) {
                        this->currentWordIndex += 1;
                        this->currentWord = this->currentSplitText[this->currentWordIndex];
                    }
                    this->wordSurface.setString(this->currentWord);
                    sf::FloatRect r = this->wordSurface.getLocalBounds();
                    this->wordSize = sf::Vector2<int>(r.width, r.height);

                    if ((this->letterX + this->wordSize.x) > this->xLimit) {
                        this->letterY += this->fontHeight + 2;
                        this->letterX = spaceLength;
                    }

                    this->checkForFlags();

                    //cout << "Checking for newlines\n";
                    if (startsWith(this->currentWord, "\n")) {  // I'm not sure if this is actually going to work
                        // Skip a line
                        if ((this->letterY + this->fontHeight * 2 + 4) < this->yLimit) {
                            this->letterY += this->fontHeight + 2;
                            this->letterX = this->spaceLength;
                            //this->letterX = 0;
                        }
                    }
                }
            }
            //cout << "Changing letter\n";
            this->currentLetter = this->currentWord[this->currentLetterIndex];
        } else if (this->paused) {
            //cout << "Pausing\n";
            this->pauseCounter += 1;  // Change this to work with a delta-time system
            if (this->pauseCounter >= this->pauseDuration) {
                this->paused = false;
                this->checkForFlags();
            }
        }
    } else if (this->stopAnimating) {  // Stop animations when text is done scrolling.
        // Fill in later
    }
    this->textSurface.display();
}
