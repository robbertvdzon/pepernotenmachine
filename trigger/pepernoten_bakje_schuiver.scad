use <pepernoten_servo_aansluiting.scad>
use <pepernoten_bakje_inlaad.scad>

difference() {
union() {
    translate([0, 0, 0])
        cube([4, 26, 60]);


    translate([0, 5.5, 17.5])
        cube([55, 15, 25]);

//    translate([52.5,36.5,48.5]) {
//        rotate([180,0,0]) 
//            pepernoten_servo_aansluiting();
//    }

//    translate([-25,0,0]) {
//        rotate([0,0,0]) 
//            pepernoten_inlaad();
//    }




    
    translate([0, -20, 0])
       cube([5, 52+20, 60]);
    
    translate([0, 5+15, 46.5-4])
        cube([21, 32, 4]);

    translate([0, 5.5+15-5, 46.5-5])
        cube([55, 4+1, 9-4]);
    
    
}


    translate([-1, 5.5+2, 19.5])
        cube([60, 11, 21]);


    translate([25, -4.5, 17.5+5])
        cube([20, 99, 15]);

    translate([45,100,30]) {
        rotate([90,0,0]) 
                cylinder(h=1000, r=7.5, $fn=100);
    }
    translate([25,100,30]) {
        rotate([90,0,0]) 
                cylinder(h=1000, r=7.5, $fn=100);
    }
    
    translate([ -49.5/2-15+52.5, -10/2+36.5, 0])
        cylinder(h=9910, r=2, center=true, $fn=40);

    translate([ -49.5/2-15+52.5,  10/2+36.5, 0])
        cylinder(h=9910, r=2, center=true, $fn=40);    
    
    
    
// aansluiting voor inlaad
 translate([-30,13-23,20]){
      rotate([0,90,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
 translate([-30,13+23,20]){
      rotate([0,90,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
 translate([-30,13-23,40]){
      rotate([0,90,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
 translate([-30,13+23,40]){
      rotate([0,90,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
    
    

}
