Parameter.txt

0.0039 0.5 0.2		//�ѤƤ��� 0.0039*(0.5*t^0.2)
thershold 0.80		//�䪺R^2����
edge error 0.01		//�䪺�ѤƮt����
PLUS fixed 7		//��l��lifetime�]�w �[fixed�N��T�w���~ �_�h�On+����
TIGHT 1.000001		//clock period���@�Ӥp���� ���Χ�
FINAL 10		//�̫�refine������
MONTE YES		//�X�a�dù�O�_�}��


AgingRate.txt
�ѤW�ӤU��1,2,3,4,5,10�~�Ѥ�
����k��20%, 40%, 80%, 50%���Ѥ�

0.0463 0.0762 0.1073 0.0797
0.0562 0.0875 0.1217 0.0915
0.0626 0.0949 0.1312 0.0993
0.0675 0.1005 0.1383 0.1051
0.0715 0.1051 0.1442 0.1099
0.0851 0.1208 0.1641 0.1263

��J���O�榡:

./research [netlist�ɦW] [timing report�ɦW] [�Ѥ����p���ɦW] [�Ʊ�lifetime] [��h�֦�shortlist] [�C��shortlist���h�֦�refinement] [n+error��error��(�i����J �w�]��0.1n)]
