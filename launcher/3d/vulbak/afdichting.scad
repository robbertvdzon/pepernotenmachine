difference() {
union() {

  cylinder(h=3, r=100/2, $fn=100, center=false);
  translate([-50, -5, 0])
        cube([4, 10, 26]);    
  translate([-50, -8, 0])
        cube([8, 16, 9]);

}

  translate([0, -100, -1])
        cube([1000, 200, 6]);
    translate([-100,0,14]){
        rotate([0,90,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }    
    translate([-100,0,22]){
        rotate([0,90,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }    

}
