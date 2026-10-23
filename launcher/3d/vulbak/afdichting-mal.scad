module afdichting_mal() {
    difference() {
        translate([0, 0, -10])
        cylinder(h=300, r=300/2, $fn=100);

        translate([0, 0, -20])
            cylinder(h=2000, r=104/2, $fn=100);
    }
}

// Alleen tonen als je dit bestand zelfstandig opent
afdichting_mal();