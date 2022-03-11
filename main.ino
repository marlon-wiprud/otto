#include <Servo.h>

int LEFT_TIP_TOE_POS = 60;
int RIGHT_TIP_TOE_POS = 120;

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
        if(current != target)
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

BodyCtrl* bodyCtrl = new BodyCtrl();

int servoPinLegLeft = 2;
int servoPinLegRight = 3;
int servoPinFootLeft = 4;
int servoPinFootRight = 5;


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

    bodyCtrl->setJoints(leftLeg, leftFoot, rightLeg, rightFoot);
    bodyCtrl->reset();
}



int run = 0;

void loop() {

    
    JointCtrl* l = bodyCtrl->leftFoot();

    // if(bodyCtrl->isTipToe())
    // {
    //     bodyCtrl->setAllTarget(90);
    // }

    // if(bodyCtrl->isHome())
    // {
    //     bodyCtrl->tipToe();
    // }

    if(run ==0)
    {
        bodyCtrl->tipToe();
        run = run + 1;
    }    

    bodyCtrl->step();
    
}