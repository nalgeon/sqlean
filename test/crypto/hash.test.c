// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Hash function tests.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crypto/blake3.h"
#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/sha2.h"
#include "crypto/xxhash.h"

// The largest digest (SHA-512).
#define MAX_DIGEST_SIZE SHA512_DIGEST_SIZE

// The longest test input.
#define MAX_INPUT_SIZE 102400

// hash_<algo>_inc computes a digest by feeding data to the incremental API
// in pieces of the given size. With empty_updates, each piece is preceded by
// an update of zero bytes, which must not change the result.
#define DEFINE_HASHER(name, ctx_type)                                            \
    static void hash_##name##_inc(const uint8_t* data, size_t len, size_t chunk, \
                                  bool empty_updates, uint8_t* out) {            \
        ctx_type ctx;                                                            \
        name##_init(&ctx);                                                       \
        for (size_t i = 0; i < len; i += chunk) {                                \
            size_t n = len - i < chunk ? len - i : chunk;                        \
            if (empty_updates) {                                                 \
                name##_update(&ctx, data + i, 0);                                \
            }                                                                    \
            name##_update(&ctx, data + i, n);                                    \
        }                                                                        \
        name##_final(&ctx, out);                                                 \
    }

DEFINE_HASHER(md5, md5_ctx)
DEFINE_HASHER(sha1, sha1_ctx)
DEFINE_HASHER(sha256, sha256_ctx)
DEFINE_HASHER(sha384, sha384_ctx)
DEFINE_HASHER(sha512, sha512_ctx)
DEFINE_HASHER(blake3, blake3_hasher)

typedef void (*OneShot)(const uint8_t* data, size_t len, uint8_t* out);
typedef void (
    *Incremental)(const uint8_t* data, size_t len, size_t chunk, bool empty_updates, uint8_t* out);

typedef struct {
    const char* algo;
    size_t len;        // input is the first len bytes of the pattern
    const char* want;  // hex digest
} HashTest;

// Golden digests computed with independent implementations (Python's hashlib,
// and the blake3 and xxhash packages) over the pattern i % 251. The lengths
// sit around the 64- and 128-byte block boundaries (MD5, SHA, BLAKE3), the
// 1024-byte BLAKE3 chunk boundaries, and the xxHash stripe and buffer sizes.
// clang-format off
static const HashTest hash_tests[] = {
    // md5
    {"md5", 0, "d41d8cd98f00b204e9800998ecf8427e"},
    {"md5", 1, "93b885adfe0da089cdf634904fd59f71"},
    {"md5", 3, "b95f67f61ebb03619622d798f45fc2d3"},
    {"md5", 15, "58b7ce493ac99c66058538dacb1e3c94"},
    {"md5", 16, "1ac1ef01e96caf1be0d329331a4fc2a8"},
    {"md5", 17, "1bdd36b0a024c90db383512607293692"},
    {"md5", 31, "e9e77893ba926e732f483282f416ffac"},
    {"md5", 32, "b4ffcb23737cec315a4a4d1aa2a620ce"},
    {"md5", 33, "5506a276a0a9acc3093f9169c73cf8c5"},
    {"md5", 55, "6912ee65fff2d9f9ce2508cddf8bcda0"},
    {"md5", 56, "51fdd1acda72405dfdfa03fcb85896d7"},
    {"md5", 63, "48a6295221902e8e0938f773a7185e72"},
    {"md5", 64, "b2d3f56bc197fd985d5965079b5e7148"},
    {"md5", 65, "8bd7053801c768420faf816fadba971c"},
    {"md5", 111, "4fad3ab7d8546851ec1bb63ea7e6f5a8"},
    {"md5", 112, "d1fec2ac3715e791ca5f489f300381b3"},
    {"md5", 127, "8402b21e7bc7906493bae0dac017f1f9"},
    {"md5", 128, "37eff01866ba3f538421b30b7cbefcac"},
    {"md5", 129, "46f986692847558fc38b0cece591c20f"},
    {"md5", 239, "4006f7d5ec5e2e49cb20bc0d8296439c"},
    {"md5", 240, "ddabc96224d832fde27d53c83270c3f1"},
    {"md5", 241, "267a256d457a3856bbfce6554c1566df"},
    {"md5", 1023, "7437d7a881387db7c41cf6930ab5e35d"},
    {"md5", 1024, "9ee0a0e0c0bc0f1ff29d663d1fdf0743"},
    {"md5", 1025, "3f3789452b88cb32b8cbfbafe715e29a"},
    {"md5", 2049, "cb7fabac9ac9fd2cb4e79a6d12c8308e"},
    {"md5", 8193, "d70940b0f2d0a8ac92655d222ce0ecd5"},
    {"md5", 102400, "1a0f81547e5ba2e9c4a4b94a74731993"},
    // sha1
    {"sha1", 0, "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
    {"sha1", 1, "5ba93c9db0cff93f52b521d7420e43f6eda2784f"},
    {"sha1", 3, "0c7a623fd2bbc05b06423be359e4021d36e721ad"},
    {"sha1", 15, "fb1d9241f67827ce6dd7ac55f1e3c4e4f50caa03"},
    {"sha1", 16, "56178b86a57fac22899a9964185c2cc96e7da589"},
    {"sha1", 17, "0a0315ec7b1e22a79fc862edf79bda2fc01669e3"},
    {"sha1", 31, "43bbacb5f62a0482cbdb564171b04365ca6e27c0"},
    {"sha1", 32, "ae5bd8efea5322c4d9986d06680a781392f9a642"},
    {"sha1", 33, "eb90bce364635c4c23b49f493f0043579bc85c17"},
    {"sha1", 55, "8ae2d46729cfe68ff927af5eec9c7d1b66d65ac2"},
    {"sha1", 56, "636e2ec698dac903498e648bd2f3af641d3c88cb"},
    {"sha1", 63, "6d942da0c4392b123528f2905c713a3ce28364bd"},
    {"sha1", 64, "c6138d514ffa2135bfce0ed0b8fac65669917ec7"},
    {"sha1", 65, "69bd728ad6e13cd76ff19751fde427b00e395746"},
    {"sha1", 111, "bc544e24573d592290fdaff8ecf3f7f2b00cd483"},
    {"sha1", 112, "e4ce142d09a84a8645338dd6535cbfaaf800d320"},
    {"sha1", 127, "89d7312a903f65cd2b3e34a975e55dbea9033353"},
    {"sha1", 128, "e6434bc401f98603d7eda504790c98c67385d535"},
    {"sha1", 129, "3352e41cc30b40ae80108970492b21014049e625"},
    {"sha1", 239, "2e4766b0ebf480a027ae6f9e59e7a2ef9e2aef2a"},
    {"sha1", 240, "f7b8e5e76e6b4cb3dfa7af7070d5560400822b65"},
    {"sha1", 241, "54717f94e8f3ded40b4cc1a470eacb25cb10136f"},
    {"sha1", 1023, "1d58257e7e9cecee00473911023732e408e9bee3"},
    {"sha1", 1024, "0ac28084ff74933d05123496dafd3791684d9b53"},
    {"sha1", 1025, "ca9fdc040579afc74c0e6314fee7af12bd5c4284"},
    {"sha1", 2049, "08f4013d7a63d68a95db3d9ac6d6e29e932f2808"},
    {"sha1", 8193, "c7fac2a2751ad1552813f366c3daef8eba512436"},
    {"sha1", 102400, "f18b928d893ae172a000efa19b80e1c04fb36414"},
    // sha256
    {"sha256", 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
    {"sha256", 1, "6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d"},
    {"sha256", 3, "ae4b3280e56e2faf83f414a6e3dabe9d5fbe18976544c05fed121accb85b53fc"},
    {"sha256", 15, "7071fc3188fde7e7e500d4768f1784bede1a22e991648dcab9dc3219acff1d4c"},
    {"sha256", 16, "be45cb2605bf36bebde684841a28f0fd43c69850a3dce5fedba69928ee3a8991"},
    {"sha256", 17, "3e5718fea51a8f3f5baca61c77afab473c1810f8b9db330273b4011ce92c787e"},
    {"sha256", 31, "4f23c2ca8c5c962e50cd31e221bfb6d0adca19111dca8e0c62598ff146dd19c4"},
    {"sha256", 32, "630dcd2966c4336691125448bbb25b4ff412a49c732db2c8abc1b8581bd710dd"},
    {"sha256", 33, "5d8fcfefa9aeeb711fb8ed1e4b7d5c8a9bafa46e8e76e68aa18adce5a10df6ab"},
    {"sha256", 55, "463eb28e72f82e0a96c0a4cc53690c571281131f672aa229e0d45ae59b598b59"},
    {"sha256", 56, "da2ae4d6b36748f2a318f23e7ab1dfdf45acdc9d049bd80e59de82a60895f562"},
    {"sha256", 63, "29af2686fd53374a36b0846694cc342177e428d1647515f078784d69cdb9e488"},
    {"sha256", 64, "fdeab9acf3710362bd2658cdc9a29e8f9c757fcf9811603a8c447cd1d9151108"},
    {"sha256", 65, "4bfd2c8b6f1eec7a2afeb48b934ee4b2694182027e6d0fc075074f2fabb31781"},
    {"sha256", 111, "60780e9451bdc43cf4530ffc95cbb0c4eb24dae2c39f55f334d679e076c08065"},
    {"sha256", 112, "09373f127d34e61dbbaa8bc4499c87074f2ddb10e1b465f506d7d70a15011979"},
    {"sha256", 127, "92ca0fa6651ee2f97b884b7246a562fa71250fedefe5ebf270d31c546bfea976"},
    {"sha256", 128, "471fb943aa23c511f6f72f8d1652d9c880cfa392ad80503120547703e56a2be5"},
    {"sha256", 129, "5099c6a56203f9687f7d33f4bfdf576d31dc91f6b695ecea38b2770c87631135"},
    {"sha256", 239, "5e1c10284710f5c2db48f88de3d051579643a1ed042afa846a7844895351a77b"},
    {"sha256", 240, "abf4bafcddb38bbf3855e47b5e61b75dedbcf42aa44ffd4bb85d0b08d97e2682"},
    {"sha256", 241, "211882aeac8a599b0a55ec280e1a978923edef69cd86541bcbd58db864c45eac"},
    {"sha256", 1023, "1c5e88a585b61754df6137d66632a7348557a88358afc401b0a0a4fc427104a9"},
    {"sha256", 1024, "2bce1ba628720664be4b9fdd77aae0678e5f0f3f02fc6ff641ec879094f6a404"},
    {"sha256", 1025, "bc0b6b10b89b9487a12fda2a8cc13194e7091c217aabf8b92846274026f4bcd0"},
    {"sha256", 2049, "26e1e2808e3a6cf967ca03f6749a063c5ed55f92f5874653a1faabed78346f00"},
    {"sha256", 8193, "7e3691790cd64b19d4edb1a80e988214515abeb53aa0f34ffbfe4b4bf405d120"},
    {"sha256", 102400, "74588b7f0bcc354ac14d9cf199fa3a20c05f0c7293b9075b2f2e146e718de800"},
    // sha384
    {"sha384", 0, "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"},
    {"sha384", 1, "bec021b4f368e3069134e012c2b4307083d3a9bdd206e24e5f0d86e13d6636655933ec2b413465966817a9c208a11717"},
    {"sha384", 3, "4f895854c1a4fc5aa2e0456eaf8d0ecaa70c196bd901153861d76b8fa3cd95ceea29eab6a279f8b08437703ce0b4b91a"},
    {"sha384", 15, "0aa75534f0f58756a01e3366f78e7611bc7f432364c649c3f50547f7bca3e5489531b8ab129495feac834ff0a0b45db6"},
    {"sha384", 16, "c81df98d9e6de9b858a1e6eba0f1a3a399d98c441e67e1062601806485bb89125efd54cc78df5fbceabc93cd7c7ba13b"},
    {"sha384", 17, "fdd3c4c0f87eec0cadd73028a06b01e67696c7e04960936b30c73f004cf6b595d644533f8b473c8e63b02d593a64b041"},
    {"sha384", 31, "04aaa180c2cd24f0fb150b1aa360f445344150dca13e1abb8117d42e25df7fe29246d9f00c7473d20cec32a71e64e1f5"},
    {"sha384", 32, "e7112491faeefd57786da73f367b25a6f5769f5c98fa7b704d8d37747724a647371989e8b0fe8d3cb23f9eedd528456b"},
    {"sha384", 33, "ea27126d0b96e00e428943ea94f4b03fd22d56c4ff4636eed139d027e6d45ef57ab86093a7342b3b3851fd3bfd1dda23"},
    {"sha384", 55, "dcedb6b590edb4efa849c801e6b6490657a5c1e64f69269f5f63c9267f6223de24cea7aaa6b267d9bcecc15147b6c875"},
    {"sha384", 56, "7b9132d597b8873ad55bbc30f18ed3f2c9f340e7de69fb5774056c71a06d9bc2b14137e9e1c68b6b645fed28b188249d"},
    {"sha384", 63, "dd66b519f51a925814407a449c60b34c553d7652d41783ee903a810a4c9f833b8181c91c7f12283eacd6a5f8a2639ddf"},
    {"sha384", 64, "9f2c9eb7116b3d7a4ba84a74a4d4eff8a5efcf54b6d7b662693c38577914c73a214766f0a175339bb0895a863824fc0a"},
    {"sha384", 65, "14b0a9ffce149426bf5045ffc24c057451d2473186deb4f150117b855911a7641651fb1e15df406eb373d71151c46f25"},
    {"sha384", 111, "f5f9fe110d809d34029de262a01b208356caec6e054c7f926b2591f6c9780579d4b59f5578c6f531a84f158a33660cef"},
    {"sha384", 112, "33ba080ec0ccb378e4e95fed3b26c23aa1a280476e007519ee47f60cd9c5c8a65d627259a9aa2fd33ca06d3c14ee5548"},
    {"sha384", 127, "d5fcfe2fcf6b3ef375ede37c8123d9b78065fecc1d55197e2f7721e6e9a93d0ba4d7fd15f9b96dea2744df24141ba2ef"},
    {"sha384", 128, "ca2385773319124534111a36d0581fc3f00815e907034b90cff9c3a861e126a741d5dfcff65a417b6d7296863ac0ec17"},
    {"sha384", 129, "ef49ae5b9ad51433d00323528d81ea8d2e4d2b507dbd9f1cb84f952b66249a788b1c89fcdb77a0db9f1feb901d47fc73"},
    {"sha384", 239, "2556cf077a788c49bb6d600f4a3cee635c4443832d169f761537afee2980742b9f34afbc87f598dd0aedc4a826ed6a73"},
    {"sha384", 240, "d64769ad58f5a338669b935f3431e5bef31667d0a2437bff78f1e5275075f434fff675f9833ea04ac4e5c2e2c2c99b8c"},
    {"sha384", 241, "3264cad70d24b53cec95269b980dab85a30d24cf8bdbd68f0ff8a45c6208f05723a4b3270cd095fb8b2d9a4167fb3d3b"},
    {"sha384", 1023, "962bb195eccbf3ffb32f1743014d6dab9603c44edaa41043e03d696d8ad69bcc678a9383c8ed868812919803096abd39"},
    {"sha384", 1024, "bd7443454c828292bdc435a095d5f71aa8724ec2ff6a0e08d25a11e0e417c6a903f566e9bdbb7b6cbfbfdc55c53ac766"},
    {"sha384", 1025, "fde60c0157845cf167a59164fd4d47085b967721548e1580428dd54161e2d359db5981f758998e8b0e933115cd7ce811"},
    {"sha384", 2049, "343e0fb036b3eaa843a4fde8392439523383c4b33af93e35c61179c8ae34c6ee05ab9ea1f8ffd255fc851e10bb667af4"},
    {"sha384", 8193, "db9915793c4d820a48fa15b50475a7f03ba5ba727af78fe03123a446101a179b731d3f56ca160d556344546fa48da7e1"},
    {"sha384", 102400, "2c63ef39f81d19e88281bd6e79f7e0e96f5e79dda4ddea49d9f552442c47820a9faadf38643e329fd9861a727b461543"},
    // sha512
    {"sha512", 0, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"},
    {"sha512", 1, "b8244d028981d693af7b456af8efa4cad63d282e19ff14942c246e50d9351d22704a802a71c3580b6370de4ceb293c324a8423342557d4e5c38438f0e36910ee"},
    {"sha512", 3, "8081da5f9c1e3d0e1aa16f604d5e5064543cff5d7bace2bb312252461e151b3fe0f034ea8dc1dacff3361a892d625fbe1b614cda265f87a473c24b0fa1d91dfd"},
    {"sha512", 15, "4236736d08f26244e75b51614091cc2c2907d5dd162f8497b14d58d0d954a777c8397549bee468f30e480252d9b893175df7d2bf415a128ccc79407d9d5fa536"},
    {"sha512", 16, "daa295beed4e2ee94c24015b56af626b4f21ef9f44f2b3d40fc41c90900a6bf1b4867c43c57cda54d1b6fd4869b3f23ced5e0ba3c05d0b1680df4ec7d0762403"},
    {"sha512", 17, "7b9ae840aab8bee45b038ce398d15a8679db92d0ba46fa67d1b8177986e41eacde915c6552fc2af8678425b8be81b57e0f7eeadcc93b56c58dfc38b4d33bf25d"},
    {"sha512", 31, "0674a3cdf5f7c18c1b7524c87c36037f3d0267512d11e052f453dbc097cfd52bc331950880cf904656c70758b2e25e21fe2c7e0462e861112a2dc9d0636bbafc"},
    {"sha512", 32, "3d94eea49c580aef816935762be049559d6d1440dede12e6a125f1841fff8e6fa9d71862a3e5746b571be3d187b0041046f52ebd850c7cbd5fde8ee38473b649"},
    {"sha512", 33, "301f1cd7b25b097ae4c79a97e92bce359d1289f6754e76b71e7617a06e7783a3cc30f5290209bda3e6af239d0dc0f3d1cd4c5e866f4c5c3209eabbd7aafb8058"},
    {"sha512", 55, "6856647f269c2ee3d8128f0b25427659d880641ef343300dd3cd4679168f58d6527fda70b4ebc854e2065e172b7d58c1536992c0810599259ba84a2b40c65414"},
    {"sha512", 56, "8b12b2f6fe400a51d29656e2b8c42a1bbfe6fcf3e425da430db05d1a2dda14790dee20fa8b22d8762afffe4988a5c98a4430d22a17e41e23d90fa61ab75671a9"},
    {"sha512", 63, "9dc9c5598e55dc42955695320839788e353f1d7f6ba74df74c80a8a52f463c0697f57f68835d1418f4ce9b6530cd79bd0f4c6f7e13c93feb1218c0b65c2c0561"},
    {"sha512", 64, "ee4320ebaf3fdb4f2c832b137200c08e235e0fa7bbd0eb1740c7063ba8a0d151da77e003398e1714a955d475b05e3e950b639503b452ec185de4229bc4873949"},
    {"sha512", 65, "02856cef735f9acec6b9e33f0fbc8f9804d2aa54187f382b8ae842e5d3696c07459aad2a5aed25ea5e117eb1c7ba35da6a7a8adce9e6afe3ad79e9fa42d5bba8"},
    {"sha512", 111, "a1a111449b198d9b1f538bad7f3fc1022b3a5b1a5e90a0bc860de8512746cbc31599e6c834de3a3235327af0b51ff57bf7acf1974a73014d9c3953812edc7c8d"},
    {"sha512", 112, "c5fbd731d19d2ae1180f001be72c2c1aaba1d7b094b3748880e24593b8e117a750e11c1bd867cc2f96dace8c8b74abd2d5c4f236be444e77d30d1916174070b9"},
    {"sha512", 127, "eab89674feaa34e27aebeeff3c0a4d70070bb872d5e9f186cf1dbbdee517b6e35724d629ff025a5b07185e911ada7e3c8acf830aa0e4f71777bd2d44f504f7f0"},
    {"sha512", 128, "1dffd5e3adb71d45d2245939665521ae001a317a03720a45732ba1900ca3b8351fc5c9b4ca513eba6f80bc7b1d1fdad4abd13491cb824d61b08d8c0e1561b3f7"},
    {"sha512", 129, "1d9da57fbbdab09afb3506ab2d223d06109d65c1c8ad197f50138f714bc4c3f2fe5787922639c680acad1c651f955990425954ce2cba0c5cc83f2667d878eb0f"},
    {"sha512", 239, "cb4c7fd522756d5781ad3a4f590a1d862906b960e7720136cb3fb36b563caa1ea5689134291fa79c80ccc2b4092b41df32ebdcb36dbe79db483440228c1622a8"},
    {"sha512", 240, "6c48466c9f6c07e4ab762c696b7eeb35cfe236fca73683e5fab873ac3489b4d2eb3d7afcce7e8165dbbf37aded3b5b0c889c0b7e0f1790a8330d8677429d91a5"},
    {"sha512", 241, "4f663484efca758d670147758a5d4d9e5933fe22c0a1dc01f954738ff8310a6515b3ec42094449075ed678c55ee001a4fb91b1081dfae6ab83860b7b4cc7b4ab"},
    {"sha512", 1023, "c260dc7074f5d12c5226aecfdb8e3f9b6457b14d980aa0d28b2082d78377ee8c33d65466d9f369cfb347e86d806ab5a9488dd93b63252341106f3604d8e96879"},
    {"sha512", 1024, "9af3eed7e9dd11428bb922c6830c32065154532303781f8ea4f20792d616703884d564ebfd2bfa65faed8fc8fd91d9e1d3f12897fbb1e2247632db70ce30573e"},
    {"sha512", 1025, "1f0cb287c12671e2f498170ff2762886686ceb88b7d63f944708d3060752376ff38e4a88ab7ceb0bb437083e7f1d051049b8d94356e72e4d59adcc102f585ac0"},
    {"sha512", 2049, "c86261239b9dbf452f0b5ba56fd312c5f3eced55cfa7057e522a378fa5a707fbababd9c7c559160dd5b64a9826cf2db5e09a99fcdb8f411c8ae2f26f9bee8395"},
    {"sha512", 8193, "825d72249c6adeff4ab01490f1dcf174bb1872bafe8deae51bb9f7ead43be084cd690306fe2b20fa7b791bf32efe814b8b06667df2eb6235ff2c1bbbfb9a0e4a"},
    {"sha512", 102400, "2acda2d1386c8cd9ef01c797cfd154b073e7ea26e4c5741e9f2aee089dc8106cb887526d5bbb04920c2b742b2dab945e2a8db4cd31f58ac945aec9df89b1f18d"},
    // blake3
    {"blake3", 0, "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"},
    {"blake3", 1, "2d3adedff11b61f14c886e35afa036736dcd87a74d27b5c1510225d0f592e213"},
    {"blake3", 3, "e1be4d7a8ab5560aa4199eea339849ba8e293d55ca0a81006726d184519e647f"},
    {"blake3", 15, "163475fc98871015d19097d29536653bdeac6a9c3a18ad4b3a629cfaa1bbf32e"},
    {"blake3", 16, "a6a492965517a830cb75fdb713465aa465f2f098233896fea44c1d98268bf9e3"},
    {"blake3", 17, "8462aa7be93b09fda7b93cf9f9cddb703f6dd2cc0c8edd5f9eee092edf8abf0c"},
    {"blake3", 31, "bda80c7fe2db38be6387b35c870bd7728d67b7b6cc5eb9b0e5c7dcb21ea754c2"},
    {"blake3", 32, "e528e95798037df410543d9f31e396ecdd458d71b157d6014398bae32fb56c65"},
    {"blake3", 33, "4f4e6c1dffd3a6c9959876d15aa96b5fb0da8632b995f6ca2e30503f2829fa29"},
    {"blake3", 55, "d04ec5f6f5e7daf5ced7a1671fbe912580a56576c8bf6a2ed4b80e35548f9c13"},
    {"blake3", 56, "60f238116f2936698a88cda03d8df79d7431249373b048ee7a063849fe6e9742"},
    {"blake3", 63, "e9bc37a594daad83be9470df7f7b3798297c3d834ce80ba85d6e207627b7db7b"},
    {"blake3", 64, "4eed7141ea4a5cd4b788606bd23f46e212af9cacebacdc7d1f4c6dc7f2511b98"},
    {"blake3", 65, "de1e5fa0be70df6d2be8fffd0e99ceaa8eb6e8c93a63f2d8d1c30ecb6b263dee"},
    {"blake3", 111, "929dcdaf9b6a6e500a34978b5c9206d0258bc190f38c9e8e42fb19a4820b2171"},
    {"blake3", 112, "c881a3c5ba84905a418f3da19726541b5bacd9e3438a741ffd980e00865fe13c"},
    {"blake3", 127, "d81293fda863f008c09e92fc382a81f5a0b4a1251cba1634016a0f86a6bd640d"},
    {"blake3", 128, "f17e570564b26578c33bb7f44643f539624b05df1a76c81f30acd548c44b45ef"},
    {"blake3", 129, "683aaae9f3c5ba37eaaf072aed0f9e30bac0865137bae68b1fde4ca2aebdcb12"},
    {"blake3", 239, "1f91ca9b6d63155d9b4f4a21aa1ebdde39f30620fe6e162273577db54131d51c"},
    {"blake3", 240, "45e1a0dc23dbe51733d7269a3c0f519c2a63b0718835b2b537677eba734db0d8"},
    {"blake3", 241, "749b36ae651c22e8567db692a6876e0ca4fd3daeb7aa8fa3ab2f642ccc69a8f6"},
    {"blake3", 1023, "10108970eeda3eb932baac1428c7a2163b0e924c9a9e25b35bba72b28f70bd11"},
    {"blake3", 1024, "42214739f095a406f3fc83deb889744ac00df831c10daa55189b5d121c855af7"},
    {"blake3", 1025, "d00278ae47eb27b34faecf67b4fe263f82d5412916c1ffd97c8cb7fb814b8444"},
    {"blake3", 2049, "5f4d72f40d7a5f82b15ca2b2e44b1de3c2ef86c426c95c1af0b6879522563030"},
    {"blake3", 8193, "bab6c09cb8ce8cf459261398d2e7aef35700bf488116ceb94a36d0f5f1b7bc3b"},
    {"blake3", 102400, "bc3e3d41a1146b069abffad3c0d44860cf664390afce4d9661f7902e7943e085"},
    // xxh32
    {"xxh32", 0, "02cc5d05"},
    {"xxh32", 1, "cf65b03e"},
    {"xxh32", 3, "663e9a55"},
    {"xxh32", 15, "57c01ece"},
    {"xxh32", 16, "b72837f4"},
    {"xxh32", 17, "7c77adc2"},
    {"xxh32", 31, "ef24f709"},
    {"xxh32", 32, "830741c1"},
    {"xxh32", 33, "f1e6a545"},
    {"xxh32", 55, "b308cf59"},
    {"xxh32", 56, "bf3049ca"},
    {"xxh32", 63, "4c80747b"},
    {"xxh32", 64, "31120435"},
    {"xxh32", 65, "a4da78a4"},
    {"xxh32", 111, "a8380c4d"},
    {"xxh32", 112, "da4d11fd"},
    {"xxh32", 127, "f07a1b8b"},
    {"xxh32", 128, "6d6194b7"},
    {"xxh32", 129, "6572cb97"},
    {"xxh32", 239, "6e3f2d0d"},
    {"xxh32", 240, "1fd0fbb0"},
    {"xxh32", 241, "5b9a61e5"},
    {"xxh32", 1023, "cc664d02"},
    {"xxh32", 1024, "69dd7c7e"},
    {"xxh32", 1025, "d6a1188a"},
    {"xxh32", 2049, "6524dcf5"},
    {"xxh32", 8193, "8ee4003d"},
    {"xxh32", 102400, "96c7eb65"},
    // xxh64
    {"xxh64", 0, "ef46db3751d8e999"},
    {"xxh64", 1, "e934a84adb052768"},
    {"xxh64", 3, "e5c7bb4533bc65dd"},
    {"xxh64", 15, "a948f5f0f6abac2d"},
    {"xxh64", 16, "44b6ef2fb84169f7"},
    {"xxh64", 17, "5603e60c527599b6"},
    {"xxh64", 31, "c346d2b59b4d8ee1"},
    {"xxh64", 32, "cbf59c5116ff32b4"},
    {"xxh64", 33, "0c535d1acafb8ead"},
    {"xxh64", 55, "511cd496994ea839"},
    {"xxh64", 56, "0cd869f80ff2b918"},
    {"xxh64", 63, "e26aa9e2a95f8e4f"},
    {"xxh64", 64, "f7c67301db6713f0"},
    {"xxh64", 65, "c31eb63b2ae4465b"},
    {"xxh64", 111, "666cc5e38345de58"},
    {"xxh64", 112, "f9a92625d5b4bccd"},
    {"xxh64", 127, "464d085810ce0199"},
    {"xxh64", 128, "7a7fe14647b9ab92"},
    {"xxh64", 129, "0ba25dfd6e891fcf"},
    {"xxh64", 239, "a08803b2cdf37175"},
    {"xxh64", 240, "012947f0da6a27b1"},
    {"xxh64", 241, "8d643f23bf2808e1"},
    {"xxh64", 1023, "d66738f081c25cf4"},
    {"xxh64", 1024, "138e26c65048ce29"},
    {"xxh64", 1025, "cfd73aedd2d6a39d"},
    {"xxh64", 2049, "27858160679416ba"},
    {"xxh64", 8193, "755e4befd10cccf4"},
    {"xxh64", 102400, "eb1adcdd9e1369a6"},
    // xxh3_64
    {"xxh3_64", 0, "2d06800538d394c2"},
    {"xxh3_64", 1, "c44bdff4074eecdb"},
    {"xxh3_64", 3, "5f4299fc161c9cbb"},
    {"xxh3_64", 15, "55ecedc2b87bb042"},
    {"xxh3_64", 16, "8355e3a6f61770db"},
    {"xxh3_64", 17, "9ef341a99de37328"},
    {"xxh3_64", 31, "4f36db8e4df378fd"},
    {"xxh3_64", 32, "3523581fe96e4c05"},
    {"xxh3_64", 33, "e68c56ba88991e58"},
    {"xxh3_64", 55, "530dd3a74f500b7a"},
    {"xxh3_64", 56, "bc77d05ff952f606"},
    {"xxh3_64", 63, "aaa5f0fb98a36ae8"},
    {"xxh3_64", 64, "6187eb9089b0ed55"},
    {"xxh3_64", 65, "6928c76ce90422d0"},
    {"xxh3_64", 111, "b0d786133f3dd93f"},
    {"xxh3_64", 112, "153e9963db232fec"},
    {"xxh3_64", 127, "120b9787f8425f2f"},
    {"xxh3_64", 128, "85c6174c7ff4c46b"},
    {"xxh3_64", 129, "ec7642b431ba3e5a"},
    {"xxh3_64", 239, "972b14e3c46f214b"},
    {"xxh3_64", 240, "375a384d957fe865"},
    {"xxh3_64", 241, "02e8cd95421c6d02"},
    {"xxh3_64", 1023, "d3d91d80ac495685"},
    {"xxh3_64", 1024, "e5d78bafa45b2aa5"},
    {"xxh3_64", 1025, "e95c42288f28186e"},
    {"xxh3_64", 2049, "6c9600c0e506e2ae"},
    {"xxh3_64", 8193, "d6735a2b792cf505"},
    {"xxh3_64", 102400, "1428e17f1cac2837"},
    // xxh3_128
    {"xxh3_128", 0, "99aa06d3014798d86001c324468d497f"},
    {"xxh3_128", 1, "a6cd5e9392000f6ac44bdff4074eecdb"},
    {"xxh3_128", 3, "e3b55f57945a17cf5f4299fc161c9cbb"},
    {"xxh3_128", 15, "301a9f754e8f569a0017ea4be19bc787"},
    {"xxh3_128", 16, "72950631827607e2842812cc870dcae2"},
    {"xxh3_128", 17, "685bc458b37d057fc06e233df7729217"},
    {"xxh3_128", 31, "4ed3946d393b687bb54de3993874ed20"},
    {"xxh3_128", 32, "25e7c9b3424ceed2457d9566b6fcd697"},
    {"xxh3_128", 33, "02175c3aabb00637e08d84951339de86"},
    {"xxh3_128", 55, "d7420506a37184c24d27405399d46ba6"},
    {"xxh3_128", 56, "9e4390d2170659904a1730efd65eb655"},
    {"xxh3_128", 63, "bb8d4c458fac1f120302a39b74a9cf52"},
    {"xxh3_128", 64, "9c6e140a465545e590c1971ddb04ce74"},
    {"xxh3_128", 65, "ebedf05eeadc28f11aee64a1615de88f"},
    {"xxh3_128", 111, "d91ad0a1d3da0d7fa839f92954ccd2d1"},
    {"xxh3_128", 112, "fb1602dc7a000b4987f6c5c881b9ea74"},
    {"xxh3_128", 127, "d5add870c9c9e00f060c2e3ddf0f2fb9"},
    {"xxh3_128", 128, "14792fc3af88dc6c05321a0b64d67b41"},
    {"xxh3_128", 129, "dd5e74ac6b45f54ebc30b63382b09a3b"},
    {"xxh3_128", 239, "9843ab31a06be0dffe21374628fcc539"},
    {"xxh3_128", 240, "65b5be86da5540e7c92b68e16f83bbb6"},
    {"xxh3_128", 241, "1da1cb61bcb8a2a102e8cd95421c6d02"},
    {"xxh3_128", 1023, "4325711b0ed4d742d3d91d80ac495685"},
    {"xxh3_128", 1024, "d0ac1f7b93bf57b9e5d78bafa45b2aa5"},
    {"xxh3_128", 1025, "2882ebca04ec915ce95c42288f28186e"},
    {"xxh3_128", 2049, "39a54bc93f74921b6c9600c0e506e2ae"},
    {"xxh3_128", 8193, "eaa446aa30f78391d6735a2b792cf505"},
    {"xxh3_128", 102400, "ecd387d36185351b1428e17f1cac2837"},
};
// clang-format on

static uint8_t pattern[MAX_INPUT_SIZE];

static void init_pattern(void) {
    for (size_t i = 0; i < sizeof(pattern); i++) {
        pattern[i] = (uint8_t)(i % 251);
    }
}

static void to_hex(const uint8_t* src, size_t len, char* dst) {
    for (size_t i = 0; i < len; i++) {
        snprintf(dst + i * 2, 3, "%02x", src[i]);
    }
    dst[len * 2] = '\0';
}

// Checks a digest against its hex representation.
static void assert_hex(const uint8_t* digest, size_t digest_size, const char* want) {
    char got[MAX_DIGEST_SIZE * 2 + 1];
    to_hex(digest, digest_size, got);
    assert(strcmp(got, want) == 0);
}

// Checks every golden digest of the algorithm, computed with the one-shot
// function and, if there is one, with the incremental API: in a single
// update and in pieces of different sizes.
static void assert_algo(const char* algo, OneShot hash, Incremental inc, size_t digest_size) {
    // pieces smaller than, equal to, and larger than the block and chunk sizes
    static const size_t chunks[] = {1, 3, 63, 64, 65, 127, 128, 129, 1000, 1024, 4097};
    uint8_t digest[MAX_DIGEST_SIZE];
    size_t n_tests = 0;
    for (size_t i = 0; i < sizeof(hash_tests) / sizeof(hash_tests[0]); i++) {
        const HashTest* test = &hash_tests[i];
        if (strcmp(test->algo, algo) != 0) {
            continue;
        }
        n_tests++;
        assert(strlen(test->want) == digest_size * 2);

        hash(pattern, test->len, digest);
        assert_hex(digest, digest_size, test->want);
        if (test->len == 0) {
            // empty input may come without a buffer
            hash(NULL, 0, digest);
            assert_hex(digest, digest_size, test->want);
        }

        if (inc == NULL) {
            continue;
        }
        size_t whole = test->len > 0 ? test->len : 1;
        inc(pattern, test->len, whole, false, digest);
        assert_hex(digest, digest_size, test->want);
        if (test->len < 1024) {
            // pieces only matter for inputs that span several blocks
            continue;
        }
        for (size_t j = 0; j < sizeof(chunks) / sizeof(chunks[0]); j++) {
            inc(pattern, test->len, chunks[j], false, digest);
            assert_hex(digest, digest_size, test->want);
        }
        inc(pattern, test->len, 100, true, digest);
        assert_hex(digest, digest_size, test->want);
    }
    assert(n_tests > 0);
}

static void test_md5(void) {
    printf("test_md5...");
    assert_algo("md5", md5_hash, hash_md5_inc, MD5_DIGEST_SIZE);
    printf("OK\n");
}

static void test_sha1(void) {
    printf("test_sha1...");
    assert_algo("sha1", sha1_hash, hash_sha1_inc, SHA1_DIGEST_SIZE);
    printf("OK\n");
}

static void test_sha256(void) {
    printf("test_sha256...");
    assert_algo("sha256", sha256_hash, hash_sha256_inc, SHA256_DIGEST_SIZE);
    printf("OK\n");
}

static void test_sha384(void) {
    printf("test_sha384...");
    assert_algo("sha384", sha384_hash, hash_sha384_inc, SHA384_DIGEST_SIZE);
    printf("OK\n");
}

static void test_sha512(void) {
    printf("test_sha512...");
    assert_algo("sha512", sha512_hash, hash_sha512_inc, SHA512_DIGEST_SIZE);
    printf("OK\n");
}

static void test_blake3(void) {
    printf("test_blake3...");
    assert_algo("blake3", blake3_hash, hash_blake3_inc, BLAKE3_OUT_LEN);
    printf("OK\n");
}

static void test_xxh32(void) {
    printf("test_xxh32...");
    assert_algo("xxh32", xxh32_hash, NULL, XXH32_DIGEST_SIZE);
    printf("OK\n");
}

static void test_xxh64(void) {
    printf("test_xxh64...");
    assert_algo("xxh64", xxh64_hash, NULL, XXH64_DIGEST_SIZE);
    printf("OK\n");
}

static void test_xxh3_64(void) {
    printf("test_xxh3_64...");
    assert_algo("xxh3_64", xxh3_64_hash, NULL, XXH3_64_DIGEST_SIZE);
    printf("OK\n");
}

static void test_xxh3_128(void) {
    printf("test_xxh3_128...");
    assert_algo("xxh3_128", xxh3_128_hash, NULL, XXH3_128_DIGEST_SIZE);
    printf("OK\n");
}

int main(void) {
    init_pattern();

    // Cryptographic hashes.
    test_md5();
    test_sha1();
    test_sha256();
    test_sha384();
    test_sha512();
    test_blake3();

    // Non-cryptographic hashes.
    test_xxh32();
    test_xxh64();
    test_xxh3_64();
    test_xxh3_128();
}
