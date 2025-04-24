const int BlockTypeAir = 0;
const int BlockTypeBedrock = 1;
const int BlockTypeDirt = 2;
const int BlockTypeGrass = 3;
const int BlockTypeLava = 4;
const int BlockTypeSnow = 5;
const int BlockTypeStone = 6;
const int BlockTypeWater = 7;

float blockTypeToFloat(int blockType)
{
    return float(blockType) * 0.1;
}

int blockTypeFromFloat(float value)
{
    return int(round(value * 10.0));
}
