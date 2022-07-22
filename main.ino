#include <Servo.h>
#include <SoftwareSerial.h> 

SoftwareSerial Blue(1, 0); // RX | TX 

int LEFT_TIP_TOE_POS = 60;
int RIGHT_TIP_TOE_POS = 120;

int MAX_POS = 160;
int MIN_POS = 20;
int SERIAL_BAUD = 9600;
int BLUETOOTH_BAUD = 9600;

class JointCtrl {
    public:
    int start = 90;
    int current = 90;
    int target = 90;
    int rate = 1;
    Servo joint;

    void attach(Servo _joint)
    {
        joint = _joint;
    }

    void reset()
    {
        joint.write(start);
    }

    int increment()
    {
        if(current + rate > target)
        {
            return target - current;
        }

        return rate;
    }

    int decrement() 
    {
       if(current - rate < target)
        {
            return current - target; 
        }
        return rate;
    }

    void step()
    {
        if(current != target && target >= MIN_POS && target <= MAX_POS)
        {
            if(current < target)
            {
                // Serial.println("stepping increment: ");
                // Serial.println(increment());
                // current = current + 1;
                current = current + increment();
            }

            if(current > target)
            {
                // Serial.println("stepping decrement: ");
                // Serial.println(decrement());
                // current = current - 1;
                current = current - decrement();
            }
            joint.write(current); 
        }
    }

    boolean hasReachedTarget()
    {   
       return current == target;
    }
};

class BodyCtrl {
    public:
    int idxLeftLeg = 0;
    int idxLeftFoot = 1;
    int idxRightLeg = 2;
    int idxRightFoot = 3;

    JointCtrl* joints[4];

    JointCtrl* leftLeg()
    {
        return joints[idxLeftLeg];
    }

    JointCtrl* leftFoot()
    {
        return joints[idxLeftFoot];
    }

    JointCtrl* rightLeg()
    {
        return joints[idxRightLeg];
    }

    JointCtrl* rightFoot()
    {
        return joints[idxRightFoot];
    }


    void setJoints(JointCtrl* _leftLeg, JointCtrl* _leftFoot, JointCtrl* _rightLeg, JointCtrl* _rightFoot)
    {
        joints[idxLeftLeg] = _leftLeg;
        joints[idxLeftFoot] = _leftFoot;
        joints[idxRightLeg] = _rightLeg;
        joints[idxRightFoot] = _rightFoot;
    }

    void reset()
    {
        for(int i = 0; i < 4; i++)
        {
            joints[i]->reset();
        }

    }

    void setAllTarget(int target)
    {
        for(int i = 0; i < 4; i++)
        {
            joints[i]->target = target;
        } 
    }

    void setTargets(int LL, int LF, int RL, int RF, int rate)
    {
        joints[idxLeftLeg]->target = LL;
        joints[idxLeftLeg]->rate = rate;
        
        
        joints[idxLeftFoot]->target = LF;
        joints[idxLeftFoot]->rate = rate;
        
        joints[idxRightLeg]->target = RL;
        joints[idxRightLeg]->rate = rate;
        
        joints[idxRightFoot]->target = RF;
        joints[idxRightLeg]->rate = rate;
        
    }

    boolean hasReachedTarget()
    {
        for(int i = 0; i < 4; i++)
        {
            if(!joints[i]->hasReachedTarget())
            {
                return false;
            }
        }
        
        return true;
    }

    void step()
    {
        for(int i = 0; i < 4; i++)
        {
            joints[i]->step();
        }
        delay(20);
    }

    void tipToe()
    {
        JointCtrl *l = leftFoot();
        l->target = LEFT_TIP_TOE_POS;

        JointCtrl *r = rightFoot();
        r->target = RIGHT_TIP_TOE_POS;
    }

    boolean isTipToe()
    {

        JointCtrl *l = leftFoot();
        JointCtrl *r = rightFoot();

        if(l->current == LEFT_TIP_TOE_POS && r->current == RIGHT_TIP_TOE_POS)
        {
            return true;
        }

        return false;
    }

    boolean isHome()
    {
       for(int i = 0; i < 4; i++)
       {
          if(joints[i]->current != joints[i]->start)
           {
              return false; 
           }
       }
       return true;
    }
};

class Action {
    public:
    int leftLegPos;
    int leftFootPos;
    int rightLegPos;
    int rightFootPos;
    int rate;

    Action() {
        leftFootPos = 90;
        leftLegPos = 90;
        rightLegPos = 90;
        rightFootPos = 90;
        rate = 1;
    }
};

class ActionLink {
    public:
    Action current;
    ActionLink* next;

    ActionLink(Action a)
    {
        current = a;
        next = nullptr;
    }       
};

class ActionManager {
    public:
    int size = 5;
    Action* queue[5];
    int top = -1;
    BodyCtrl* bodyCtrl; 
    boolean done;

    ActionLink* head;
    ActionLink* tail;

    ActionManager()
    {
        head = nullptr;
        tail = nullptr;
        bodyCtrl = nullptr;
        done = false;
    }
    
    void next()
    {
        boolean reachedTargets = bodyCtrl->hasReachedTarget();
        if(reachedTargets)
        {   
            Action* a = pop();
            if(a != nullptr)
            {
               Serial.println("setting targets...");
               Serial.println(a->leftLegPos);
               Serial.println(a->leftFootPos);
               Serial.println(a->rightLegPos);
               Serial.println(a->rightFootPos);
               Serial.println(a->rate);
               done = false;
               bodyCtrl->setTargets(a->leftLegPos, a->leftFootPos, a->rightLegPos, a->rightFootPos, a->rate);
            } else {
                done = true;
            }
        }

        bodyCtrl->step();
    }

    void add(Action action)
    {
        done = false;
        ActionLink* link = new ActionLink(action);
        link->current = action;

        if(head == nullptr)
        {
            Serial.println("setting first link...");
            head = link;
            tail = link;
        } else {
            Serial.println("appending link to tail...");
            tail->next = link;
            tail = link;
        }

    }

    Action* pop()
    {
        if(head == nullptr || tail == nullptr)
        {
            return nullptr;
        }

        ActionLink* tmp = head;
        if(head->next == nullptr)
        {
            Serial.println("setting head and tail to null");
            head = nullptr;
            tail = nullptr;
        }else{
            head = head->next;
        }

       return &tmp->current;
    }

};

ActionManager* actionManager = new ActionManager();

int servoPinLegRight = 3;
int servoPinLegLeft = 4;
int servoPinFootRight = 5;
int servoPinFootLeft = 6;

// ultrasonic
int trigPin = 8;    // TRIG pin
int echoPin = 9;    // ECHO pin

float duration_us, distance_cm;


Action actionBallerina()
{
    Action a = Action();
    a.leftFootPos = 60;
    a.leftLegPos = 120;
    a.rightFootPos = 120;
    a.rightLegPos = 60;
    return a;
}  

Action actionReverseBallerina()
{
    Action a = Action();
    a.leftFootPos = 120;
    a.leftLegPos = 60;
    a.rightFootPos = 60;
    a.rightLegPos = 120;
    return a;
}  


Action actionHipFlexionOut()
{
    Action a = Action();
    a.leftFootPos = 90;
    a.leftLegPos = 120;
    a.rightFootPos = 90;
    a.rightLegPos = 60;
    return a;
}


Action actionHipFlexionIn()
{
    Action a = Action();
    a.leftFootPos = 90;
    a.leftLegPos = 60;
    a.rightFootPos = 90;
    a.rightLegPos = 120;
    return a;
}

Action actionTipToe()
{
    Action a = Action();
    a.leftFootPos = 120;
    a.leftLegPos = 90;
    a.rightFootPos = 60;
    a.rightLegPos = 90;
    return a;
}

Action actionRightTipToe()
{
    Action a = Action();
    a.rightFootPos = 60;
    a.rightLegPos = 90;
    return a;
}

Action actionLeftTipToe()
{
    Action a = Action();
    a.leftFootPos = 120;
    a.leftLegPos = 90;
    return a;
}

Action actionReverseTipToe()
{
    Action a = Action();
    a.leftFootPos = 60;
    a.leftLegPos = 90;
    a.rightFootPos = 120;
    a.rightLegPos = 90;
    return a;
}


Action actionReset()
{
    Action a = Action();
    a.leftFootPos = 90;
    a.leftLegPos = 90;
    a.rightFootPos = 90;
    a.rightLegPos = 90; 
    return a;
}

Action actionRightLeg(int hip, int foot) 
{
    Action a = actionReset();
    a.rightLegPos = hip;
    a.rightFootPos = foot;
    return a;
}

Action actionLeftLeg(int hip, int foot) 
{
    Action a = actionReset();
    a.leftLegPos = hip;
    a.leftFootPos = foot;
    return a;
}

Action actionLeftFoot(int pos) {
    Action a = actionReset();
    a.leftFootPos = pos;
    return a;
}

void movementTestFeet()
{
    actionManager->add(actionTipToe()); 
    actionManager->add(actionReverseTipToe());
    actionManager->add(actionReset()); 
}

void movementTestHips()
{
    actionManager->add(actionHipFlexionIn());
    actionManager->add(actionHipFlexionOut());
    actionManager->add(actionReset());
}

void movementTestAllJoints()
{
    actionManager->add(actionBallerina());
    actionManager->add(actionReset());
    actionManager->add(actionReverseBallerina());
    actionManager->add(actionReset());
}

void movementWalk(int steps) 
{
    for(int i = 0; i < steps; i++)
    {

        Action r = actionRightLeg(120, 70);
        r.leftLegPos = 120;
        r.leftFootPos = 70;

        Action l = actionLeftLeg(60, 110);
        l.rightLegPos = 60;
        l.rightFootPos = 110;

        actionManager->add(r);
        actionManager->add(l);
   }
   actionManager->add(actionReset());
}

void movementWalkBackwards(int steps) 
{
    for(int i = 0; i < steps; i++)
    {

        Action r = actionRightLeg(60, 110);
        r.rightLegPos = 60;
        r.rightFootPos = 110;

        Action l = actionLeftLeg(120, 70);
        l.leftLegPos = 120;
        l.leftFootPos = 70;


        actionManager->add(r);
        actionManager->add(l);
   }
   actionManager->add(actionReset());
}


void routineTestJoints() {
    movementTestFeet();
    movementTestHips();
    movementTestAllJoints();
    movementTestFeet();
    movementTestHips();
    movementTestAllJoints();
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    Blue.begin(BLUETOOTH_BAUD);

    Servo servoLeftLeg;
    Servo servoLeftFoot;
    Servo servoRightLeg;
    Servo servoRightFoot;

    servoLeftLeg.attach(servoPinLegLeft);
    servoLeftFoot.attach(servoPinFootLeft);
    servoRightLeg.attach(servoPinLegRight);
    servoRightFoot.attach(servoPinFootRight);

    JointCtrl* leftLeg = new JointCtrl();
    JointCtrl* leftFoot = new JointCtrl();
    JointCtrl* rightLeg = new JointCtrl();
    JointCtrl* rightFoot = new JointCtrl();


    leftLeg->attach(servoLeftLeg);
    leftFoot->attach(servoLeftFoot);
    rightLeg->attach(servoRightLeg);
    rightFoot->attach(servoRightFoot); 

    BodyCtrl* bodyCtrl = new BodyCtrl();
    bodyCtrl->setJoints(leftLeg, leftFoot, rightLeg, rightFoot);

    actionManager->bodyCtrl = bodyCtrl;
    
    // configure the trigger pin to output mode
    pinMode(trigPin, OUTPUT);
    // configure the echo pin to input mode
    pinMode(echoPin, INPUT);

    actionManager->add(actionReset());
}

float getDistance()
{
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration_us = pulseIn(echoPin, HIGH);

    distance_cm = 0.017 * duration_us;

    return distance_cm;
}

void react_close()
{
        long n = random(4);
 
        switch (n)
        {
        case 0:
            actionManager->add(actionTipToe());
            break;
        
        case 1:
            actionManager->add(actionLeftTipToe());
            break;
        
        case 2:
            actionManager->add(actionRightTipToe());
            break;
        
        case 3:
            actionManager->add(actionReverseTipToe());
            break;
        }

        actionManager->add(actionReset());
}

int DISTANCE_CLOSE = 10;
int DISTANCE_MED = 20;
int DISTANCE_FAR = 30;

bool is_close = false;
bool is_medium = false;
bool is_far = false;

void loop() {

    if(Blue.available()) {
        Serial.println("AVAILABLE");
    }else{
        Serial.println("UNAVAILABLE -->");
    }

    float d = getDistance();
    
    if(d < DISTANCE_CLOSE)
    {
        if(!is_close)
        {
            // react_close();
        }

        is_close = true;
        is_far = false;
        is_medium = false;
    }

    if(d > DISTANCE_CLOSE && d < DISTANCE_FAR)
    {   
        if(is_close)
        {
            // movementWalk(2);
        }

        is_close = false;
        is_medium = true;
        is_far = false;
    }

    if(d > DISTANCE_FAR)
    {
        if(!is_far)
        {
            // actionManager->add(actionReverseTipToe());
            // actionManager->add(actionReset());
        }

        is_close = false;
        is_medium = false;
        is_far = true;
    }

    // while(!actionManager->done)
    // {
    //     // actionManager->next();   
    // }
    

}