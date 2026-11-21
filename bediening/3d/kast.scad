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
     cube([80, 22, 8]);
     translate([40, -5, 4])  rotate([0, 90, 0]) cylinder(h=80, d=3, center=true, $fn=40);
     translate([40, 22+5, 4])  rotate([0, 90, 0]) cylinder(h=80, d=3, center=true, $fn=40);
    
 }

// Voor een vlakke DXF/SVG-export: vervang onderstaande regel door paneel_2d();
    difference() {
        union() {
            kast();
        }
        union() {
            translate([-20,((plaat_hoogte-22)/2),25]) uitsparing_power();

            // power
            translate([-20,((plaat_hoogte)/2+40),28]) rotate([0, 90, 0])  cylinder(h=80, d=6, center=true, $fn=40);
            // montage
            translate([-20,((plaat_hoogte)/2+40),42]) rotate([0, 90, 0])  cylinder(h=999, d=3, center=true, $fn=40);
            translate([-20,((plaat_hoogte)/2-40),42]) rotate([0, 90, 0])  cylinder(h=999, d=3, center=true, $fn=40);
            
        }
    }
