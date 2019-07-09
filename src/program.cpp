#include "program.h"

Program::Program(string programType) {
    this->programType = programType;
    this->color = sf::Color::White;
    this->load();
    this->currentMove = 0;
    this->size = 0;
}

Program::Program(Program* original) {  // Copy constructor
    this->programType = original->programType;
    this->screenName = original->screenName;
    this->spriteCoord = original->spriteCoord;
    this->color = original->color;

    for (ProgramSector* s : original->sectors) {
        this->sectors.push_back(new ProgramSector(s->coord));
    }

    for (int i=0; i<this->sectors.size(); i++) {
        ProgramSector* s = this->sectors[i];
        vector<sf::Vector2i> linkCoords;
        vector<int> linkIndices;
        for (ProgramSector* l : s->links) {
            linkCoords.push_back(l->coord);
            for (int j=0; j<this->sectors.size(); j++) {
                if (this->sectors[j]->coord == l->coord) {
                    linkIndices.push_back(j);
                }
            }
        }
        for (int index : linkIndices) {
            ProgramSector* sectorToLink = this->sectors[index];
            ProgramSector::linkSectors(s, sectorToLink);
        }
    }

    this->actions = original->actions;
    this->description = original->description;
    this->size = original->size;
    this->maxSize = original->maxSize;
    this->speed = original->speed;
    this->maxSpeed = original->speed;
    this->currentMove = original->currentMove;
    this->state = original->state;


}

Program::Program(DataBattlePiece* original) {  // Alt copy constructor
    this->programType = original->programType;
    this->screenName = original->screenName;
    this->spriteCoord = original->spriteCoord;
    this->color = original->color;

    for (ProgramSector* s : original->sectors) {
        this->sectors.push_back(new ProgramSector(s->coord));
    }

    for (int i=0; i<this->sectors.size(); i++) {
        ProgramSector* s = this->sectors[i];
        vector<sf::Vector2i> linkCoords;
        vector<int> linkIndices;
        for (ProgramSector* l : s->links) {
            linkCoords.push_back(l->coord);
            for (int j=0; j<this->sectors.size(); j++) {
                if (this->sectors[j]->coord == l->coord) {
                    linkIndices.push_back(j);
                }
            }
        }
        for (int index : linkIndices) {
            ProgramSector* sectorToLink = this->sectors[index];
            ProgramSector::linkSectors(s, sectorToLink);
        }
    }

    this->actions = original->actions;
    this->description = original->description;
    this->size = original->size;
    this->maxSize = original->maxSize;
    this->speed = original->speed;
    this->maxSpeed = original->speed;
    this->currentMove = original->currentMove;
    this->state = original->state;


}

Program::~Program() {
    // Fill this in, deallocate any sectors.
    cout << "Program deconstructor called\n";
    for (int i=0; i<this->sectors.size(); i++) {
        delete this->sectors[i];
    }
}

void Program::load() {
    ifstream textFile;
    textFile.open("Data\\Programs\\" + programType + ".txt");
    string line;
    vector<string> splitLine;
    while (getline(textFile, line)) {
        splitLine = splitString(line, ':');
        if (startsWith(line, "name")) {
            this->name = splitLine[1];
            this->screenName = splitLine[1];
        } else if (startsWith(line, "screenName")) {
            this->screenName = splitLine[1];
        } else if (startsWith(line, "maxSize")) {
            this->maxSize = stoi(splitLine[1]);
        } else if (startsWith(line, "speed")) {
            this->speed = stoi(splitLine[1]);
        } else if (startsWith(line, "maxSpeed")) {
            this->maxSpeed = stoi(splitLine[1]);
        } else if (startsWith(line, "action")) {
            this->actions.push_back(ACTION_DB[splitLine[1]]);
        } else if (startsWith(line, "sprite")) {
            this->spriteCoord = *(new sf::Vector2i(stoi(splitLine[1]), stoi(splitLine[2])));
        } else if (startsWith(line, "maxSize")) {
            this->color = *(new sf::Color(stoi(splitLine[1]), stoi(splitLine[2]), stoi(splitLine[3])));
        } else if (startsWith(line, "description")) {
            this->description = splitLine[1];
        } else if (startsWith(line, "color")) {
            this->color = *(new sf::Color(stoi(splitLine[1]), stoi(splitLine[2]), stoi(splitLine[3])));
        }
    }
}

void Program::move(sf::Vector2i coord, bool firstTime=false) {
    //cout << "Moving\n";
    if (firstTime) {
        sectors = *(new vector<ProgramSector*>);
        sectors.push_back(new ProgramSector(coord));
        this->size = 1;
    } else {
        bool doublingBack = false;
        int doubleBackIndex = -1;
        for (int i=0; i<this->sectors.size(); i++) {
            if (this->sectors[i]->coord == coord) {
                doublingBack = true;
                doubleBackIndex = i;
            }
        }

        if (doublingBack) {
            //cout << "Doubling back\n";
            // Swap the positions of the current head and the sector we're going to
            ProgramSector* doubleBackSector = this->sectors[doubleBackIndex];
            this->sectors[doubleBackIndex] = this->sectors[0];
            this->sectors[0] = doubleBackSector;
            this->currentMove++;
        } else {
            //cout << "Not doubling back\n";
            this->sectors.insert(this->sectors.begin(), new ProgramSector(coord, this->sectors[0]));  // Add new sector as the head
            //cout << "Added new head\n";
            while (this->sectors.size() > this->maxSize) {  // Trim program down if necessary
                //cout << "Trimming program down\n";
                //cout << "Size: " << this->sectors.size() << '\n';
                //cout << "Max size: " << this->maxSize << '\n';
                for (int i=0; i<this->sectors.size(); i++) {
                    ProgramSector* s = this->sectors[i];
                    if ((s->numLinks == 1) && (i != 0)) {
                        //cout << "Found singly-linked sector: (" << s->coord.x << ", " << s->coord.y << ")\n";
                        ProgramSector* connectedSector = s->links[0];
                        // Unlink the sectors
                        for (int j=0; j<connectedSector->links.size(); j++) {
                            //cout << "connectedSector: (" << connectedSector->coord.x << ", " << connectedSector->coord.y << ")\n";
                            if (connectedSector->links[j]->coord == s->coord) {
                                connectedSector->links.erase(connectedSector->links.begin() + j);
                                connectedSector->numLinks--;
                                //cout << connectedSector->numLinks << '\n';
                                break;
                            }
                        }
                        // Delete the sector from the program
                        delete this->sectors[i];
                        this->sectors.erase(this->sectors.begin() + i);
                        break;
                    }
                }
            }
            this->currentMove++;
        }
        this->size = this->sectors.size();
    }
}

void Program::addSector(sf::Vector2i coord, int pos=0) {
    vector<ProgramSector*>::iterator itr = sectors.begin();
    advance(itr, pos);
    ProgramSector* newSector = new ProgramSector(coord, sectors[pos]);
    sectors.push_back(newSector);
    this->size++;
}

void Program::useAction(Netmap_Playable* level, int actionIndex, sf::Vector2i targetCoord) {
    cout << "useAction called\n";
    this->actions[actionIndex]->use(level, this, targetCoord);
    // Action is used whether it failed or not, end the turn
    this->state = 'd';
}

void Program::switchToAiming(int actionIndex) {
    this->state = 'a';
    this->currentActionIndex = actionIndex;
    this->currentAction = this->actions[this->currentActionIndex];
    // Should add an if statement to automatically end the turn if we don't have any actions (like the Memory Hog)
}

void Program::noAction() {
    this->state = 'd';
    this->currentAction = nullptr;
    this->currentActionIndex = -1;
}

void Program::takeDamage(int damage) {
    cout << "Program::takeDamage() called\n";
    for (int d=0; d<damage; d++) {  // For each point of damage
        if (this->size > 1) {
            // Loop through the sectors.  Like trimming off stuff in move().
            for (int i=0; i<this->sectors.size(); i++) {
                ProgramSector* s = this->sectors[i];
                if ((s->numLinks == 1) && (i != 0)) {  // Find a singly-linked sector that isn't the head
                    ProgramSector* connectedSector = s->links[0];
                    // Unlink the sectors
                    for (int j=0; j<connectedSector->links.size(); j++) {
                        if (connectedSector->links[j]->coord == s->coord) {
                            connectedSector->links.erase(connectedSector->links.begin() + j);
                            connectedSector->numLinks--;
                            break;
                        }
                    }
                    // Delete the sector from the program
                    delete this->sectors[i];
                    this->sectors.erase(this->sectors.begin() + i);
                    break;
                }
            }
            this->size = this->sectors.size();
        } else {
            // Kill the program
            cout << "X_X\n";
            // Not sure what to do here, so I'll just set the state to 'x' (dead) and do other stuff in DataBattle::play()
            this->state = 'x';
        }
    }
}

void Program::grow(Netmap_Playable* level, int amtToGrow) {
    for (int i=0; i<amtToGrow; i++) {
        if (this->size < this->maxSize) {  // If we haven't hit max size
            sf::Vector2i coordToAttach;
            ProgramSector* previousSector;

            for (ProgramSector* s : this->sectors) {
                // See if there's a sector we can grow off of.  Check if any of the contiguous sectors are free.
                sf::Vector2i coord = s->coord;
                if (startsWith(level->lookAt(sf::Vector2<int>(coord.x, coord.y-1)), "tile")) {  // North
                    coordToAttach = sf::Vector2<int>(coord.x, coord.y-1);
                    previousSector = s;
                    break;
                } else if (startsWith(level->lookAt(sf::Vector2<int>(coord.x, coord.y+1)), "tile")) {  // South
                    coordToAttach = sf::Vector2<int>(coord.x, coord.y+1);
                    previousSector = s;
                    break;
                } else if (startsWith(level->lookAt(sf::Vector2<int>(coord.x+1, coord.y)), "tile")) {  // East
                    coordToAttach = sf::Vector2<int>(coord.x+1, coord.y);
                    previousSector = s;
                    break;
                } else if (startsWith(level->lookAt(sf::Vector2<int>(coord.x-1, coord.y)), "tile")) {  // West
                    coordToAttach = sf::Vector2<int>(coord.x-1, coord.y);
                    previousSector = s;
                    break;
                }
            }
            if (previousSector != nullptr) {  // If we found an empty contiguous sector
                ProgramSector* newSector = new ProgramSector(coordToAttach, previousSector);
                this->sectors.push_back(newSector);
                this->size = this->sectors.size();
            } else {  // We found no such contiguous sector and can't grow further
                break;
            }

        } else {  // If we have hit max size, we can't grow any further
            break;
        }
    }
}

void Program::prepForTurn() {
    this->state = 'm';
    this->currentActionIndex = -1;
    this->currentAction = nullptr;
    this->currentMove = 0;
}
