function rdfile(infile, graph)
	XX=dlmread(infile, '', 1, 0);
	time = XX(:,1) - XX(1,1);
	XX(:, 1)=[];
	medval = median(XX,2);
	Tcorr = XX - medval;
	plot(time, Tcorr);
	legend("0-10","0-11","0-20","0-21","0-30","0-31","0-40","0-41","0-50","0-51","0-60","0-61","0-70","0-71", ...
			"1-10","1-11","1-20","1-21","1-30","1-31","1-40","1-41","1-50","1-51","1-60","1-61","1-70","1-71");
	xlabel("time, s"); ylabel("T, {}^\\circ{}C");
	plotgr(sprintf("%s_corrected", graph));
	Corrections = median(Tcorr);
	printf("Correction values:\n");
	for x = 1:size(Corrections,2)
		if(x != 1) printf(", "); endif;
		printf("%g", Corrections(x));
	endfor
	printf("\n");
	plot(time, Tcorr - Corrections);
	legend("0-10","0-11","0-20","0-21","0-30","0-31","0-40","0-41","0-50","0-51","0-60","0-61","0-70","0-71", ...
			"1-10","1-11","1-20","1-21","1-30","1-31","1-40","1-41","1-50","1-51","1-60","1-61","1-70","1-71");
	xlabel("time, s"); ylabel("T, {}^\\circ{}C");
	plotgr(sprintf("%s_zeroed", graph));
endfunction
