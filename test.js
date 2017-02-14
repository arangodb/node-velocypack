var vpack = require('./index.js');

var documents = [
  { "dog": "arfarfarf"
  , "jsteeman" : "was muss ich machen??"
  , "numbers" : [ 4711, 23, 42 ]
  , "programmer" : { "languages" : [ "rust", "cpp", "go", "pyton", "js"]
                   , "food" : [ "coffee", "fruits" ,"steaks" ]
                   , "min_iq" : 150
                   }
  }
, { "query": "FOR x IN 1..5 RETURN x"}
]

vpack.decode(new Buffer([11,37,3,49,70,112,111,116,97,116,97,51,243,23,59,10,0,0,0,0,0,50,75,95,85,76,79,49,100,45,79,45,45,45,11,3,21]));

documents.forEach(function(doc){
  vpack_buffer = vpack.encode(doc)
  console.log("buffer:");
  console.log(vpack_buffer);
  console.log("js object:");
  console.log(vpack.decode(vpack_buffer));
})
