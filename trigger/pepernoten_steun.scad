
difference() {
union() {
    
    translate([3, 22, 0])
       cube([5, 30, 46.5-4]);
    
    translate([3, 7+15, 46.5-4])
        cube([15, 30, 4]);
    translate([3, 7+15, 0])
        cube([15, 30, 4]);

    
}


    
    translate([ -49.5/2-15+52.5, -10/2+36.5, 0])
        cylinder(h=9910, r=2, center=true, $fn=40);

    translate([ -49.5/2-15+52.5,  10/2+36.5, 0])
        cylinder(h=9910, r=2, center=true, $fn=40);    
    
    translate([-5,22+15+5,27]){
      rotate([0,90,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
    translate([-5,22+15-5,27]){
      rotate([0,90,0]){
           cylinder(h=1000, r=2, $fn=100, center=false);
      }
  }
    

}
