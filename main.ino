#include <Servo.h>

int LEFT_TIP_TOE_POS = 60;
int RIGHT_TIP_TOE_POS = 120;

int MAX_POS = 160;
int MIN_POS = 20;

class JointCtrl {
    public:
    int start = 90;
    int current = 90;
    int target = 90;
    float rate = 1;
    Servo joint;

    void attach(Servo _joint)
    {
        joint = _joint;
    }

    void reset()
    {
        joint.write(start);
    }

    void step()
    {
        if(current != target && target >= MIN_POS && target <= MAX_POS)
        {
            if(current < target)
            {
                current = current + 1;
            }

            if(current > target)
            {
                current = current - 1;
            }
            joint.write(current); 
        }
    }

    boolean hasReachedTarget()
    {   
        // Serial.println("has reached: ");
        // Serial.println(current);
        // Serial.println(target);
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

    void setTargets(int LL, int LF, int RL, int RF)
    {
        joints[idxLeftLeg]->target = LL;
        joints[idxLeftFoot]->target = LF;
        joints[idxRightLeg]->target = RL;
        joints[idxRightFoot]->target = RF;
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

    ActionLink* head;
    ActionLink* tail;

    ActionManager()
    {

    }
    
    void next()
    {
        boolean done = bodyCtrl->hasReachedTarget();
        if(done)
        {   
            Action* a = pop();
            if(a != nullptr)
            {
               Serial.println("setting targets...");
            //    Serial.println(a->actionName);
               Serial.println(a->leftLegPos);
               Serial.println(a->leftFootPos);
               Serial.println(a->rightLegPos);
               Serial.println(a->rightFootPos);
               bodyCtrl->setTargets(a->leftLegPos, a->leftFootPos, a->rightLegPos, a->rightFootPos);
            } 
        }

        bodyCtrl->step();
    }

    void add(Action action)
    {
        ActionLink* link = new ActionLink(action);
        link->current = action;

        if(head == nullptr)
        {
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

int servoPinLegLeft = 2;
int servoPinLegRight = 3;
int servoPinFootLeft = 4;
int servoPinFootRight = 5;

Action actionTipToe()
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

void setup() {
    Serial.begin(9600);
    
    Servo servoLeftLeg;
    Servo servoLeftFoot;
    Servo servoRightLeg;
    Servo servoRightFoot;

  // put your setup code here, to run once:
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
    bodyCtrl->reset();

    actionManager->bodyCtrl = bodyCtrl;

    actionManager->add(actionTipToe()); 
    actionManager->add(actionReset());
    actionManager->add(actionTipToe()); 
    actionManager->add(actionReset());
    actionManager->add(actionTipToe());
    actionManager->add(actionReset());
 
}


void loop() {
    actionManager->next();   
}