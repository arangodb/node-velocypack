var vpack = require('bindings')('node-velocypack');

var test_vpack_buffer = vpack.encode(
{ "dog": "arfarfarf"
, "jsteeman" : "was muss ich machen??"
, "numbers" : [ 4711, 23, 42 ]
, "programmer" : { "languages" : [ "rust", "cpp", "go", "pyton", "js"]
                 , "food" : [ "coffee", "fruits" ,"steaks" ]
                 , "min_iq" : 150
                 }
});

console.log("buffer:");
console.log(test_vpack_buffer);
console.log("js object:");
console.log(vpack.decode(test_vpack_buffer));
