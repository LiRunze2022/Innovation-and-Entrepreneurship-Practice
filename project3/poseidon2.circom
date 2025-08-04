pragma circom 2.0.0;

template Sbox(){
    signal input in;
    signal output out;

    signal x1;
    signal x2;
    signal x3;

    x1 <== in * in;
    x2 <== x1 * in;
    x3 <== x2 * in;
    out <== x3 * in;
}

template Poseidon2_oneround(round_num, t, total, roundtype){
    signal input in[t];
    signal output out[t];

    var MDS[3][3];
    MDS[0][0] = 2;
    MDS[0][1] = 1;
    MDS[0][2] = 1;

    MDS[1][0] = 1;
    MDS[1][1] = 2;
    MDS[1][2] = 1;

    MDS[2][0] = 1;
    MDS[2][1] = 1;
    MDS[2][2] = 2;

    var roundConstants[total][t]; 
    for (var r = 0; r < total; r++) { 
        for (var i = 0; i < t; i++) {
            roundConstants[r][i] = (r * 3 + i + 1); 
        }
    }

    // ADD ROUND CONSTANT
    signal state_ac[t];
    for(var i = 0; i < t; i++)
    {
        state_ac[t] <== in[i] + roundConstants[round_num][i];
    }

    // sbox
    signal state_sbox[t];

    component sbox_function[t];

    if(roundtype == 0)
    {
        for(var i = 0; i < t; i++)
        {
            sbox_function[i] = Sbox();

            sbox_function[i].in <== state_ac[i];
            state_sbox[i] <== sbox_function[i].out;
        }
    }else{
        sbox_function[0] = Sbox();

        sbox_function[0].in <== state_ac[0];
        state_sbox[0] <== sbox_function[0].out;

        for(var i = 1; i < t; i++)
        {
            state_sbox <== state_ac[i];
        }
    }
 
    // MDS
    for(var i = 0; i < t; i++)
    {
        out[i] <== (MDS[i][0] * state_sbox[0]) + (MDS[i][1] * state_sbox[1]) + (MDS[i][2] * state_sbox[2]);
    }
}

template Poseidon2(){
    //参数为（256,3,5），完整轮8,部分轮56
    var FULL_ROUND = 8;
    var PARTIAL_ROUND = 56;
    var TOTAL_ROUNF = FULL_ROUND + PARTIAL_ROUND;
    var T = 3;

    signal input private_input[T - 1];
    signal input out;

    signal states[TOTAL_ROUNF + 1][T];

    states[0][0] <== private_input[0];
    states[0][1] <== private_input[1];
    states[0][2] <== 0;

    //过轮函数
    component rounds[TOTAL_ROUNF];

    for(var r = 0; r < TOTAL_ROUNF; r++)
    {
        var roundtype = (r < (FULL_ROUND / 2)) || (r >= TOTAL_ROUNF - (FULL_ROUND / 2)) ? 0 : 1;
        rounds[r] = Poseidon2_oneround(r, T, TOTAL_ROUNF, roundtype);

        for(var i = 0; i < T; i++)
        {
            rounds[r].in[i] <== states[r][i];
        }

        for(var i = 0; i < T; i++)
        {
            states[r + 1][i] <== rounds[r].out[i];
        }
    }

    out === states[TOTAL_ROUNF][0];
}

component main = Poseidon2();
