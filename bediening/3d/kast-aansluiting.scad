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

//
// Voor een vlakke DXF/SVG-export: vervang onderstaande regel door paneel_2d();
    difference() {
        union() {
//            kast();
            translate([3, plaat_hoogte/2-2.5, 47])
                cube([plaat_breedte-6, 5, 3]);

            translate([3, 10, 35])
                cube([3, plaat_hoogte-20, 15]);
            translate([plaat_breedte-6, 10, 35])
                cube([3, plaat_hoogte-20, 15]);
            translate([6,((plaat_hoogte)/2+40),42]) rotate([0, 90, 0])  cylinder(h=4, d=15, center=true, $fn=40);
            translate([6,((plaat_hoogte)/2-40),42]) rotate([0, 90, 0])  cylinder(h=4, d=15, center=true, $fn=40);

            translate([plaat_breedte-6,((plaat_hoogte)/2+40),42]) rotate([0, 90, 0])  cylinder(h=4, d=15, center=true, $fn=40);
            translate([plaat_breedte-6,((plaat_hoogte)/2-40),42]) rotate([0, 90, 0])  cylinder(h=4, d=15, center=true, $fn=40);

        }
        union() {
//            // montage
            translate([-20,((plaat_hoogte)/2+40),42]) rotate([0, 90, 0])  cylinder(h=999, d=3, center=true, $fn=40);
            translate([-20,((plaat_hoogte)/2-40),42]) rotate([0, 90, 0])  cylinder(h=999, d=3, center=true, $fn=40);
            
        }
    }
