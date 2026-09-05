
difference() {
union() {

    translate([-5, -10,0])
        cube([15, 49, 3]);

 translate([4,0,0]){
      rotate([0,0,0]){
           cylinder(h=8, r=3.5, $fn=100, center=false);
      }
     
  }
  
    
}
      translate([0,29,-1]){
      rotate([0,0,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
    translate([0,19,-1]){
      rotate([0,0,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }


}