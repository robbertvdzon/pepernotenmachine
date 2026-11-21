plaat_breedte = 136;
plaat_hoogte = 146;
hoekradius = 10;
plaat_dikte = 3; 
totale_hoogte = 50;
$fn = 96;
module buitenvorm_2d() {
    translate([hoekradius, hoekradius])
        offset(r = hoekradius)
            square([plaat_breedte - 2*hoekradius,
                    plaat_hoogte - 2*hoekradius]);
}
module binnenvorm_2d() {
    translate([hoekradius, hoekradius])
        offset(r = hoekradius)
            square([plaat_breedte - 2*plaat_dikte - 2*hoekradius,
                    plaat_hoogte - 2*plaat_dikte - 2*hoekradius]);
}

module kast() {
    difference() {
        linear_extrude(height = totale_hoogte) buitenvorm_2d();
        translate([plaat_dikte, plaat_dikte, plaat_dikte])linear_extrude(height = 99) binnenvorm_2d();
    }
}

module uitsparing_power(){
     translate([-2,-2.5,-10]) cube([10, 5,10]);
 
     translate([40, -16, 0]) rotate([0, 90, 0]) cylinder(h=80, d=4, center=true, $fn=40);
     translate([40, 16, 0])  rotate([0, 90, 0]) cylinder(h=80, d=4, center=true, $fn=40);
     translate([40, 0, 0]) rotate([0, 90, 0]) cylinder(h=80, d=5, center=true, $fn=40);

    
 }

// Voor een vlakke DXF/SVG-export: vervang onderstaande regel door paneel_2d();
    difference() {
        union() {
            translate([0,-10,2.5]) cube([10, 20,3]);


            translate([0,-16,-8]) cube([1.5, 32, 16]);
            translate([1, -16, 0]) rotate([0, 90, 0]) cylinder(h=1.5, d=16, center=true, $fn=40);
            translate([1, 16, 0])  rotate([0, 90, 0]) cylinder(h=1.5, d=16, center=true, $fn=40);
            translate([1, -8, 0]) rotate([0, 90, 0]) cylinder(h=1.5, d=16, center=true, $fn=40);
            translate([1, 8, 0])  rotate([0, 90, 0]) cylinder(h=1.5, d=16, center=true, $fn=40);
        }
        union() {
  
            translate([-2,0,0]) uitsparing_power();
            translate([7, -6, 0]) rotate([0, 0, 0]) cylinder(h=80, d=3, center=true, $fn=40);
            translate([7, 6, 0])  rotate([0, 0, 0]) cylinder(h=80, d=3, center=true, $fn=40);
            
        }
    }
