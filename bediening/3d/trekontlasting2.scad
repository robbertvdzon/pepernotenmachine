    difference() {
        union() {
            translate([3,-10,2.5]) cube([7, 20,3]);
        }
        union() {
  
            translate([7, -6, 0]) rotate([0, 0, 0]) cylinder(h=80, d=3, center=true, $fn=40);
            translate([7, 6, 0])  rotate([0, 0, 0]) cylinder(h=80, d=3, center=true, $fn=40);
            
        }
    }
