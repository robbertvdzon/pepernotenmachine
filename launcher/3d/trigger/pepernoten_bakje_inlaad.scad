module pepernoten_inlaad() {
difference() {
union() {
    translate([0, 0, 0])
        cube([26, 26, 60]);
    translate([0, -20, 0])
        cube([26, 26+40, 5]);
    translate([22, -20, 0])
        cube([4, 26+40, 60]);

    translate([-10, 22, 0])
        cube([10, 4, 60]);


    
}

    translate([2, 2, -2])
        cube([22, 22, 100]);
    translate([10, 13-4-4, 20])
        cube([200, 16, 20]);

    translate([10, 13-4, 20-4])
        cube([200, 8, 28]);

 translate([10,13-4,20]){
      rotate([0,90,0]){
           cylinder(h=1000, r=4, $fn=100, center=false);
      }
  }
 translate([10,13+4,20]){
      rotate([0,90,0]){
           cylinder(h=1000, r=4, $fn=100, center=false);
      }
  }
 translate([10,13-4,40]){
      rotate([0,90,0]){
           cylinder(h=1000, r=4, $fn=100, center=false);
      }
  }
 translate([10,13+4,40]){
      rotate([0,90,0]){
           cylinder(h=1000, r=4, $fn=100, center=false);
      }
  }


 translate([12,13-23,-5]){
      rotate([0,0,0]){
           cylinder(h=1000, r=2.5, $fn=100, center=false);
      }
  }
 translate([12,13+23,-5]){
      rotate([0,0,0]){
           cylinder(h=1000, r=2.5, $fn=100, center=false);
      }
  }


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



 translate([-5,100,55]){
      rotate([90,0,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
 translate([-5,100,45]){
      rotate([90,0,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
 translate([-5,100,35]){
      rotate([90,0,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
 translate([-5,100,25]){
      rotate([90,0,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }




}
}

pepernoten_inlaad();