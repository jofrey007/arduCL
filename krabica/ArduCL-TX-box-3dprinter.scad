df=0.25;
st=2;
$fn=50;

txBoxVyska = 60+df;
txBoxSirka = 44.5+df;
txBoxHlbka = 19+st;
//predny panel

ppDlzka = 140+st*2;
ppVyska = txBoxVyska+st*2;
ppHrubka = st;

module switch()
{
   rotate([0,0,180])union()
   {
      cylinder(h=st,d=6);
      translate([-2.5/2,6+1,0]) cube([2.5,2,st]);
   }
}

module pot()
{
   cylinder(h=st,d=7);
   translate([-8.4-df,(-2.8-df)/2,0]) cube([1.2+df,2.8+df,st]);
   
}

module OnOffSwitch()
{
   cylinder(h=st,d=2.2+df);   
   translate([0,19,0]) cylinder(h=st,d=2.2+df);   
   translate([-6.3/2,(19-15)/2,0]) cube([6.3,15,st]);
}

module OnOffSwitchKolik()
{
   module kolik()
   difference()
   {
      cylinder(h=st+2,d=3+df);   
      cylinder(h=st+2,d=1+df);   
   }
   translate ([0,0,-st-2]) kolik();
   translate ([0,19,-st-2]) kolik();
}

module rotaryEncoder()
{
   cylinder(h=st,d=3+df);   
   translate([0,13.7,0])cylinder(h=st,d=3+df);
   translate([7.6,7.7,0])cylinder(h=st,d=6+df);
}

module LCD()
{
   cylinder(h=st,d=3+df);
   translate([42.78,-0.7,0])cylinder(h=st,d=3+df);
   translate([43.20,40,0])cylinder(h=st,d=3+df);
   translate([0,40,0])cylinder(h=st,d=3+df);
   translate([2.22-df/2,2.8-df/2,0])cube([40+df,34+df,st]);
}

module txBox()
{
   module vyrez()
   {
      color("red") cube([10,2,17.3]);
   }
   difference()
   {
      cube([txBoxHlbka,txBoxSirka+st*2,txBoxVyska+st*2]);      
      translate([0,st,st]) cube([txBoxHlbka,txBoxSirka,txBoxVyska]);
      translate([txBoxHlbka-10-2,0,23.50]) vyrez();  
      translate([txBoxHlbka-10-2,txBoxSirka+st,23.50]) vyrez();  
   }
   difference()
   {
      cube([st,txBoxSirka+st*2,txBoxVyska+st*2]);
      translate([0,txBoxSirka-11+st,0]) cube([st,11,22.5]);
   }
   translate([-5,0,st]) cube([5,35,st]);
   translate([-5,0,txBoxVyska]) cube([5,txBoxSirka+st,st]);
}

module prednyPanel()
{
   difference()
   {
      cube([ppDlzka,ppVyska,ppHrubka]);
      translate([77,9+5,0]) LCD();
      translate([50.40,35+2,0]) rotaryEncoder();
      translate([135,txBoxVyska/2-(19/2),0]) OnOffSwitch();
      translate([33,40.8+7,0]) pot(); //flaps
      translate([18,40.8+7,0]) switch(); //TC
      translate([18,13+7,0]) switch(); //CH3
      translate([38,13+7,0]) switch(); //CH4
      translate([58,13+7,0]) switch(); //CH5      
   }
   translate([0,st,-5]) cube([ppDlzka,st,5]);
   translate([0,ppVyska-st*2,-5]) cube([ppDlzka,st,5]);
   translate([135,txBoxVyska/2-(19/2),0]) OnOffSwitchKolik();

}

module lavyPanel()
{
    translate([st,txBoxSirka+st*2,0]) rotate([90,0,270])
    union(){
       difference()
         {
            color("red") cube([txBoxSirka+st*2,txBoxVyska+st*2,st]);
            translate([(txBoxSirka+st*2)/2,(txBoxVyska+st*2)/2,0]) pot();
         }
      translate([st,st,-5]) cube([txBoxSirka,st,5]);
      translate([st,txBoxVyska,-5]) cube([txBoxSirka,st,5]);
   }
}

module uchyt()
{
   translate([3,0,0])  cube([st,10,st+df]);
   translate([0,0,st+df]) cube([5,10,st]);
}

module zadnyPanel()
{
   module kolik()
   {
      rotate([90,0,0]) union()
      {
         cylinder(h=4,d=2);
         translate([0,0,2]) cylinder(h=2,d=4);
      }
   }
   module drziakAku()
   {
      cube([75,3,10]);
      translate([0,25+3,0]) cube([75,3,10]);
      translate([75,0,0]) cube([3,25+3+3,10]);
      translate([15,0,7]) kolik();
      translate([60,0,7]) kolik();
      translate([60,25+3+3,7]) rotate([180,0,0]) kolik();
      translate([15,25+3+3,7]) rotate([180,0,0]) kolik();
   }
   cube([ppDlzka-st,ppVyska,ppHrubka]);
   translate([30,15,0]) drziakAku();
}

module spodnyPanel()
{
   difference()
   {
      cube([ppDlzka-st,txBoxSirka,st]);
      color("black") translate([((ppDlzka-st/2)-60)/2,(txBoxSirka/2)-3.2/2+10,0]) cube([60,3.2,st]);
   }
   translate([2,6,st]) uchyt();
   translate([2,30,st]) uchyt();
   translate([140,25,st]) rotate([0,0,180])uchyt();
   translate([130,2,st]) rotate([0,0,90])uchyt();
   translate([25,2,st]) rotate([0,0,90])uchyt();
   translate([60,2,st]) rotate([0,0,90])uchyt();
   translate([95,2,st]) rotate([0,0,90])uchyt();
}

module hornyPanel()
{
   cube([ppDlzka-st,txBoxSirka,st]);
   translate([2,16,0]) rotate([180,0,0]) uchyt();
   translate([2,40,0]) rotate([180,0,0]) uchyt();
   translate([140,33,0]) rotate([180,0,180])uchyt();
   translate([140,8,0]) rotate([180,0,180])uchyt();
   translate([122,2,0]) rotate([180,0,90])uchyt();
   translate([15,2,0]) rotate([180,0,90])uchyt();
   translate([45,2,0]) rotate([180,0,90])uchyt();
   translate([85,2,0]) rotate([180,0,90])uchyt();
}

module frontLR()
{
   color("red") translate([ppDlzka,0,0]) txBox();
   translate([0,st,0]) rotate([90,0,0]) prednyPanel();
   lavyPanel();
}


module backBT()
{
   color("blue") translate([st,txBoxSirka+2*st,0]) rotate([90,0,0]) zadnyPanel();
   color("blue") translate([st,st,0]) spodnyPanel();
   color("blue") translate([st,st,txBoxVyska+st]) hornyPanel();
}

frontLR();
//backBT();
//OnOffSwitch();
//OnOffSwitchKolik();
